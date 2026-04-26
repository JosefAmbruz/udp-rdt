#include "../../include/protocol/RdtSender.hpp"

#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
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

  state = State::SYN_SENT;
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

      // TODO: implement
    }
  } catch (const std::exception &e) {
    // Corrupted packet received -> Ignore it
    std::cerr << "[DEBUG] Ignored packet: " << e.what() << "\n";
  }
}

void RdtSender::handle_io_event() {}

void RdtSender::handle_timeout() {}

void RdtSender::handle_interrupt() {}

void RdtSender::slide_window() {}

void RdtSender::send_packet(Packet &pkt) {}

void RdtSender::initiate_teardown() {}
