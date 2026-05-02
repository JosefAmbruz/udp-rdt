/**
 * @file TimerManager.hpp
 * @brief Manages retransmission and global timeouts for the RDT protocol.
 *
 * This class implements the RTO calculation as specified in RFC 6298,
 * including smoothed RTT (SRTT), RTT variation (RTTVAR), and exponential backoff.
 * It also tracks a global timeout to terminate stalled connections.
 */
#pragma once

#include <chrono>
#include <optional>
#include <ratio>

class TimerManager {
public:
  /**
   * @brief Constructs a TimerManager with a specific global timeout.
   * @param global_timeout Maximum time without progress before termination.
   */
  explicit TimerManager(std::chrono::seconds global_timeout);

  /**
   * @brief Registers that protocol progress occurred, resetting the global timeout.
   *
   * Should be called whenever a new valid packet (ACK or data) is received.
   */
  void register_progress();

  /**
   * @brief Checks if the global timeout has been reached.
   * @return true if no progress has been made for global_timeout seconds.
   */
  bool has_global_timeout_expired() const;

  /**
   * @brief Updates the Smoothed RTT and RTT Variance variables based on a new measurement.
   *
   * Follows RFC 6298 formulas.
   * @param measured_rtt The time between sending a packet and receiving its ACK.
   */
  void update_rtt(std::chrono::milliseconds measured_rtt);

  /**
   * @brief Doubles the current retransmission timeout (exponential backoff).
   *
   * Called when a packet is retransmitted due to timeout.
   */
  void backoff_rto();

  /**
   * @brief Returns the current calculated Retransmission TimeOut.
   * @return The RTO in milliseconds.
   */
  std::chrono::milliseconds get_current_rto() const;

  /**
   * @brief Sets the absolute time point when the next retransmission should occur.
   * @param deadline The steady_clock time point for the timeout.
   */
  void set_retransmit_deadline(std::chrono::steady_clock::time_point deadline);

  /**
   * @brief Clears the retransmission deadline (e.g., when all packets are ACKed).
   */
  void clear_retransmit_deadline();

  /**
   * @brief Calculates how many milliseconds remain until the next timeout (RTO or global).
   *
   * Used as the timeout value for poll().
   * @return Number of milliseconds, or 0 if a timeout has already expired.
   */
  int get_next_timeout_ms() const;

private:
  std::chrono::seconds global_timeout;
  std::chrono::steady_clock::time_point last_progress_time;

  // RFC 6298 variables
  std::chrono::milliseconds srtt{0};
  std::chrono::milliseconds rttvar{0};

  // Initial RTO
  std::chrono::milliseconds rto{500};
  bool first_measurement = true;

  // Tracks when the Sender needs to wake up to retransmit a packet
  std::optional<std::chrono::steady_clock::time_point> retransmit_deadline;
};
