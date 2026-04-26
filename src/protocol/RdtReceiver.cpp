#include "../../include/protocol/RdtReceiver.hpp"

#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <system_error>
#include <unistd.h>

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
  std::vector<uint8_t> buffer;
  struct sockaddr_storage src_addr;
  socklen_t src_addr_len;

  try {
    return;

  } catch (const std::exception &e) {
    // TODO:Log silently or ignore corrupted packets
  }
}

void RdtReceiver::handle_io_event() {
  // Should never be called since get_io_fd() returns -1
}

void RdtReceiver::handle_timeout() {}

void RdtReceiver::handle_interrupt() {}

void RdtReceiver::send_ack(uint32_t ack_num, uint16_t extra_flags) {}

uint32_t RdtReceiver::generate_connection_id() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFF);
  return dist(gen);
}
