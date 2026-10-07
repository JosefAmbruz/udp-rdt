#pragma once

#include "core/Config.hpp"
#include "net/UdpSocket.hpp"
#include "protocol/TimerManager.hpp"

#include <cstdint>

class RdtEndpoint {
public:
  /**
   * @brief Constructor for shared dependencies
   */
  RdtEndpoint(UdpSocket &socket, TimerManager &timer_manager,
              const Config &config);

  /**
   * @brief Virtual destructor to ensure correct cleanup upon teardown of
   * derived classes.
   */
  virtual ~RdtEndpoint() = default;

  // INTERFACE

  /**
   * @brief Returns the local file descriptor for stdin or file I/O monitoring
   * @returns The file descriptor or -1 if the endpoint does not require local
   * I/O
   */
  virtual int get_io_fd() const = 0;

  /**
   * @brief Checks if the protocol has completely finished, including teardown
   * @returns True if the app can safely exit
   */
  virtual bool is_transfer_complete() const = 0;

  /**
   * @brief Handles incoming datagrams ready to read from the UDP socket.
   */
  virtual void handle_network_event() = 0;

  /**
   * @brief Handles data availability from the local input file descriptor.
   */
  virtual void handle_io_event() = 0;

  /**
   * @brief Handles timeout events triggered by poll() expiration (RTO or global).
   */
  virtual void handle_timeout() = 0;

  /**
   * @brief Handles termination signals by sending an abort (RST) packet to the peer.
   */
  virtual void handle_interrupt() = 0;

protected:
  // Shared state between both Sender and Receiver
  UdpSocket &socket;
  TimerManager &timer_manager;
  const Config &config;

  // The unique id established during three-way handshake
  // prevention of cross talk
  uint32_t connection_id = 0;
};
