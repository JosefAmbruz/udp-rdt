#include "../../include/protocol/RdtReceiver.hpp"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>

using namespace std::chrono;

RdtReceiver::RdtReceiver(UdpSocket &socket, TimerManager &timer_manager,
                         const Config &config)
    : RdtEndpoint(socket, timer_manager, config) {

  // Setup output destination
  if (config.get_output_file().has_value() &&
      config.get_output_file().value() != "-") {
    // Open file for writing, create if it doesn't exist, truncate if it does
    io_fd = ::open(config.get_output_file().value().c_str(),
                   O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (io_fd < 0) {
      throw std::system_error(errno, std::system_category(),
                              "Failed to open output file");
    }
  } else {
    io_fd = STDOUT_FILENO; // Default to standard output
  }
}

RdtReceiver::~RdtReceiver() {
  // Flush to ensure all data is written, then safely close the file
  if (io_fd > 0 && io_fd != STDOUT_FILENO) {
    ::fsync(io_fd);
    ::close(io_fd);
  }
}

int RdtReceiver::get_io_fd() const {
  // The receiver only listens to the network, it does not wait to read
  // local files
  return -1;
}

bool RdtReceiver::is_transfer_complete() const {
  return state == State::CLOSED;
}

void RdtReceiver::handle_network_event() {

  /**
   * | State       | Next States
   * |-------------|
   * | LISTEN      | SYN_RCVD
   * | SYN_RCVD    | ESTABLISHED
   * | ESTABLISHED |
   * | LAST_ACK    |
   * | CLOSED      |
   */

  std::vector<uint8_t> buffer;
  struct sockaddr_storage src_addr;
  socklen_t src_addr_len;

  try {
    socket.receive(buffer, src_addr, src_addr_len);
    Packet pkt = Packet::deserialize(buffer);

    // Progress happened, else we would catch
    timer_manager.register_progress();

    // First packet
    if (state == State::LISTEN && pkt.has_flag(Packet::FLAG_SYN)) {
      socket.set_target(src_addr, src_addr_len);
      connection_id = generate_connection_id();

      last_sent_control_pkt.connection_id = connection_id;
      last_sent_control_pkt.seq_num = 0;
      last_sent_control_pkt.ack_num = pkt.seq_num;
      last_sent_control_pkt.set_flag(Packet::FLAG_SYN);
      last_sent_control_pkt.set_flag(Packet::FLAG_ACK);

      auto serialized = last_sent_control_pkt.serialize();
      socket.send(serialized);

      waiting_for_control_ack = true;
      last_sent_time = steady_clock::now();
      timer_manager.set_retransmit_deadline(last_sent_time +
                                            timer_manager.get_current_rto());

      state = State::SYN_RCVD;
      std::cerr << "[RECEIVER] SYN received. Sending SYN ACK id: "
                << connection_id << "\n";
      return;
    }

    // Verify the established connection_id
    if (state != State::LISTEN && pkt.connection_id != connection_id) {
      return; // Ignore
    }

    switch (state) {
    case State::SYN_RCVD:
      if (pkt.has_flag(Packet::FLAG_ACK)) {
        // Handshake completed
        waiting_for_control_ack = false;
        timer_manager.clear_retransmit_deadline();
        state = State::ESTABLISHED;
        std::cerr << "[RECEIVER] Connection established";
      }
      // There might be a case, where ACK is lost and the client is already
      // sending data packets in which case the data packets need to be
      // processed
      [[fallthrough]];
    case State::ESTABLISHED:
      if (pkt.has_flag(Packet::FLAG_FIN)) {
        // Teardown initiated by sender
        send_ack(pkt.seq_num);

        // Send FIN immediately
        last_sent_control_pkt = Packet();
        last_sent_control_pkt.connection_id = connection_id;
        last_sent_control_pkt.set_flag(Packet::FLAG_FIN);

        auto serialized = last_sent_control_pkt.serialize();
        socket.send(serialized);

        waiting_for_control_ack = true;
        last_sent_time = steady_clock::now();
        timer_manager.set_retransmit_deadline(last_sent_time +
                                              timer_manager.get_current_rto());

        state = State::LAST_ACK;
      } else if (!pkt.payload.empty()) {
        // ACK the received packet
        send_ack(pkt.seq_num);

        // Check if it is a packet we expect or out of order one.
        if (pkt.seq_num == rcv_base) {
          // Write to disk
          ssize_t written =
              ::write(io_fd, pkt.payload.data(), pkt.payload.size());
          if (written < 0) {
            throw std::system_error(errno, std::system_category(),
                                    "Failed to write data");
          }

          rcv_base++;
          flush_buffer(); // Write any buffered continuous packets
        } else if (pkt.seq_num > rcv_base &&
                   (pkt.seq_num - rcv_base) <= WINDOW_SIZE) {
          // Packet is within window but out of order -> save it
          out_of_order_buffer[pkt.seq_num] = std::move(pkt.payload);
        }

        // If pkt.seq_num < rcv_base its a duplicate and we already sent ACK
        // above.
      }
      break;
    case State::LAST_ACK:
      if (pkt.has_flag(Packet::FLAG_ACK)) {
        waiting_for_control_ack = false;
        timer_manager.clear_retransmit_deadline();
        state = State::CLOSED;
        std::cerr << "[RECEIVER] Transfer completed. Connection closed.";
      }
      break;
    default:
      break;
    }

  } catch (const std::exception &e) {
    // TODO:Log silently or ignore corrupted packets
  }
}

void RdtReceiver::handle_io_event() {
  // Should never be called since get_io_fd() returns -1
}

void RdtReceiver::handle_timeout() {
  if (timer_manager.has_global_timeout_expired()) {
    std::cerr << "Error: Global timeout exceeded. Terminating connection";
    exit(1);
  }

  // RTO Expired
  // Retransmit Server side control packets if lost
  if (waiting_for_control_ack) {
    auto now = steady_clock::now();
    if ((now - last_sent_time) >= timer_manager.get_current_rto()) {
      auto serialized = last_sent_control_pkt.serialize();
      socket.send(serialized);

      last_sent_time = now;
      timer_manager.backoff_rto();
      timer_manager.set_retransmit_deadline(now +
                                            timer_manager.get_current_rto());
    }
  }
}

void RdtReceiver::handle_interrupt() {
  if (state != State::CLOSED && state != State::LISTEN) {
    Packet rst_pkt;
    rst_pkt.connection_id = connection_id;
    rst_pkt.set_flag(Packet::FLAG_RST);
    auto serialized = rst_pkt.serialize();
    socket.send(serialized);
  }
}

void RdtReceiver::send_ack(uint32_t ack_num, uint16_t extra_flags) {
  Packet ack;
  ack.connection_id = connection_id;
  ack.ack_num = ack_num;
  ack.set_flag(Packet::FLAG_ACK | extra_flags);

  auto serialized = ack.serialize();
  socket.send(serialized);
}

void RdtReceiver::flush_buffer() {
  // Write any continuous packets waiting in the out of order buffer
  while (out_of_order_buffer.find(rcv_base) != out_of_order_buffer.end()) {
    const auto &payload = out_of_order_buffer[rcv_base];

    ssize_t written = ::write(io_fd, payload.data(), payload.size());
    if (written < 0) {
      throw std::system_error(errno, std::system_category(),
                              "Failed to write buffered data");
    }

    out_of_order_buffer.erase(rcv_base);
    rcv_base++;
  }
}

uint32_t RdtReceiver::generate_connection_id() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFF);
  return dist(gen);
}
