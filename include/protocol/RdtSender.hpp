#pragma once

#include "../net/Packet.hpp"
#include "RdtEndpoint.hpp"
#include "TimerManager.hpp"

#include <chrono>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <sys/types.h>

class RdtSender : public RdtEndpoint {
public:
  RdtSender(UdpSocket &socket, TimerManager &timer_manager,
            const Config &config);
  ~RdtSender() override;

  int get_io_fd() const override;
  bool is_transfer_complete() const override;
  void handle_network_event() override;
  void handle_io_event() override;
  void handle_timeout() override;
  void handle_interrupt() override;

private:
  enum class State { SYN_SENT, ESTABLISHED, FIN_SENT, FIN_ACK_WAIT, CLOSED };

  struct WindowSlot {
    Packet packet;
    bool is_acked = false;
    std::chrono::steady_clock::time_point last_sent_time;
    int retries = 0;
  };

  State state = State::SYN_SENT;

  // 0 stdin, other files
  int io_fd = -1;
  bool eof_reached = false;

  static constexpr size_t WINDOW_SIZE = 64;
  std::deque<WindowSlot> window;
  uint32_t send_base = 0;
  uint32_t next_seq_num = 0;

  void send_packet(Packet &pkt);
  void read_and_send_data();
  void slide_window();
  void initiate_teardown();
};
