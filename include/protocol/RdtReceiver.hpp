#pragma once

#include "../dbg.h"
#include "../net/Packet.hpp"
#include "RdtEndpoint.hpp"

#include <chrono>
#include <map>
#include <vector>

class RdtReceiver : public RdtEndpoint {
public:
  RdtReceiver(UdpSocket &socket, TimerManager &timer_manager,
              const Config &config);
  ~RdtReceiver() override;

  int get_io_fd() const override;
  bool is_transfer_complete() const override;
  void handle_network_event() override;
  void handle_io_event() override;
  void handle_timeout() override;
  void handle_interrupt() override;

private:
  enum class State { LISTEN, SYN_RCVD, ESTABLISHED, LAST_ACK, CLOSED };

  State state = State::LISTEN;

  // 1 for stdout, or a file
  int io_fd = -1;

  static constexpr size_t WINDOW_SIZE = 64;
  uint32_t rcv_base = 1; // Seq number we expect

  // Buffer for packets that arrive out of order.
  // Key - Sequence Number
  std::map<uint32_t, std::vector<uint8_t>> out_of_order_buffer;

  // variables for handling server side retransmissions
  // like SYN-ACK or fin
  Packet last_sent_control_pkt;
  bool waiting_for_control_ack = false;
  std::chrono::steady_clock::time_point last_sent_time;

  // Helper methods
  void send_ack(uint32_t ack_num, uint16_t extra_flags = 0);
  void flush_buffer();
  uint32_t generate_connection_id();
};
