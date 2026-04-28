#include "../../include/protocol/RdtSender.hpp"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std::chrono;

RdtSender::RdtSender(UdpSocket &socket, TimerManager &timer_manager,
                     const Config &config)
    : RdtEndpoint(socket, timer_manager, config) {
  if (config.get_input_file().has_value() &&
      config.get_input_file().value() != "-") {
    io_fd = ::open(config.get_input_file().value().c_str(), O_RDONLY);
    if (io_fd < 0) {
      throw std::system_error(errno, std::system_category(),
                              "Failed to open input file");
    }
  } else {
    io_fd = STDIN_FILENO; // Default to stdin
  }

  // Initiating 3-Way handshake
  Packet syn_pkt;
  syn_pkt.set_flag(Packet::FLAG_SYN);
  syn_pkt.seq_num = 0;
  // TODO: Maybe randomize the connection_id later

  send_packet(syn_pkt);

  WindowSlot slot;
  slot.packet = syn_pkt;
  slot.last_sent_time = steady_clock::now();
  window.push_back(slot);

  state = State::SYN_SENT; // Already should be set
  next_seq_num = 1;
}

RdtSender::~RdtSender() {
  if (io_fd > 0 && io_fd != STDIN_FILENO) {
    ::close(io_fd);
  }
}

int RdtSender::get_io_fd() const {
  if (state == State::ESTABLISHED && !eof_reached &&
      window.size() < WINDOW_SIZE) {
    return io_fd;
  }

  return -1;
}

bool RdtSender::is_transfer_complete() const { return state == State::CLOSED; }

void RdtSender::handle_network_event() {
  std::vector<uint8_t> buffer;
  struct sockaddr_storage src_addr;
  socklen_t src_addr_len;

  try {
    socket.receive(buffer, src_addr, src_addr_len);
    Packet ack_pkt = Packet::deserialize(buffer);

    // Reset the global timeout
    timer_manager.register_progress();

    if (state == State::SYN_SENT) {
      if (ack_pkt.has_flag(Packet::FLAG_SYN) &&
          ack_pkt.has_flag(Packet::FLAG_ACK)) {
        // Handshake completed, received SYN ACK
        connection_id = ack_pkt.connection_id;

        // Reply with ACK
        Packet ack;
        ack.connection_id = connection_id;
        ack.set_flag(Packet::FLAG_ACK);
        send_packet(ack);

        // Clear the SYN from window
        window.pop_front();
        send_base = 1;
        state = State::ESTABLISHED;
        std::cerr << "[SENDER] Connection established, id: " << connection_id
                  << "\n";
      }
    }

    else if (state == State::ESTABLISHED || state == State::FIN_SENT) {
      if (ack_pkt.connection_id != connection_id) {
        return;
      }

      if (ack_pkt.has_flag(Packet::FLAG_ACK)) {
        uint32_t acked_seq = ack_pkt.ack_num;

        // FInd the packet in window and mark it
        for (auto &slot : window) {
          if (slot.packet.seq_num == acked_seq && !slot.is_acked) {
            slot.is_acked = true;

            auto rtt = duration_cast<milliseconds>(steady_clock::now() -
                                                   slot.last_sent_time);
            timer_manager.update_rtt(rtt);
            break;
          }
        }

        slide_window();

        // If eof reached and sent all data, start teardown
        if (state == State::ESTABLISHED && eof_reached && window.empty()) {
          initiate_teardown();
        }
      }

      // Handle server FIN
      if (ack_pkt.has_flag(Packet::FLAG_FIN)) {
        Packet fin_ack;
        fin_ack.connection_id = connection_id;
        fin_ack.ack_num = ack_pkt.seq_num;
        fin_ack.set_flag(Packet::FLAG_ACK);
        send_packet(fin_ack);

        state = State::CLOSED;
        std::cerr << "[SENDER] Transfer complete, connection closed.\n";
      }
    }
  } catch (const std::exception &e) {
    // Corrupted packet received -> Ignore it
    std::cerr << "[DEBUG] Ignored packet: " << e.what() << "\n";
  }
}

void RdtSender::handle_io_event() {
  if (state != State::ESTABLISHED || eof_reached ||
      window.size() >= WINDOW_SIZE) {
    return;
  }

  std::vector<uint8_t> payload_buf(Packet::MAX_PAYLOAD_SIZE); // 1180
  // Read chunk from file
  ssize_t bytes_read = ::read(io_fd, payload_buf.data(), payload_buf.size());

  if (bytes_read < 0) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to read input data");
  } else if (bytes_read == 0) {
    eof_reached = true;
    if (window.empty()) {
      initiate_teardown();
    }
    return;
  }

  payload_buf.resize(bytes_read);

  Packet data_pkt;
  data_pkt.connection_id = connection_id;
  data_pkt.seq_num = next_seq_num++;
  data_pkt.payload = std::move(payload_buf);

  send_packet(data_pkt);

  WindowSlot slot;
  slot.packet = data_pkt;
  slot.last_sent_time = steady_clock::now();
  window.push_back(slot);
}

void RdtSender::handle_timeout() {
  if (timer_manager.has_global_timeout_expired()) {
    std::cerr << "Error: Global timeout exceeded. Terminating connection\n";
    exit(1);
  }

  // Retrannsmission time out expired -> retransmit unacked packets
  auto now = steady_clock::now();
  auto rto = timer_manager.get_current_rto();
  bool retransmitted = false;

  for (auto &slot : window) {
    if (!slot.is_acked && (now - slot.last_sent_time) >= rto) {
      send_packet(slot.packet);
      slot.last_sent_time = steady_clock::now();
      slot.retries++;
      retransmitted = true;
    }
  }

  if (retransmitted) {
    // Backoff
    timer_manager.backoff_rto();
  }
}

void RdtSender::handle_interrupt() {
  if (state != State::CLOSED) {
    Packet rst_pkt;
    rst_pkt.connection_id = connection_id;
    rst_pkt.set_flag(Packet::FLAG_RST);
    send_packet(rst_pkt);
  }
}

void RdtSender::slide_window() {
  while (!window.empty() && window.front().is_acked) {
    window.pop_front();
    send_base++;
  }

  // Update TimerManager with the time from the oldest unacked packet
  if (!window.empty()) {
    timer_manager.set_retransmit_deadline(window.front().last_sent_time +
                                          timer_manager.get_current_rto());

  } else {
    timer_manager.clear_retransmit_deadline();
  }
}

void RdtSender::send_packet(Packet &pkt) {
  auto data = pkt.serialize();
  socket.send(data);
}

void RdtSender::initiate_teardown() {
  Packet fin_pkt;
  fin_pkt.connection_id = connection_id;
  fin_pkt.seq_num = next_seq_num++;
  fin_pkt.set_flag(Packet::FLAG_FIN);

  send_packet(fin_pkt);

  WindowSlot slot;
  slot.packet = fin_pkt;
  slot.last_sent_time = steady_clock::now();
  window.push_back(slot);

  state = State::FIN_SENT;
}
