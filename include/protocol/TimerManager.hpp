// TODO: documentation in this file
#pragma once

#include <chrono>
#include <optional>
#include <ratio>

class TimerManager {
public:
  explicit TimerManager(std::chrono::seconds global_timeout);

  /**
   * @brief Registers that protocol progress occured, resseting the
   * global_timeout.
   */
  void register_progress();

  bool has_global_timeout_expired() const;

  // Dynamic retransmission timeout
  // RFC 6298
  // Round Trip Time (RTT) of successfully ACKed packets is continuously
  // measured. The baseline retransmission timeout is then determined as
  // (Avg(RTT) + 4 * Variance). If a packet times out, the waiting time period
  // for the next attempt for that specific packet is doubled. This hopefully
  // reduces the load on congested network until it can recover.

  /**
   * @brief Updates the Smoothed RTT and RTT Variance variables based on a new
   * measurement.
   */
  void update_rtt(std::chrono::milliseconds measured_rtt);

  /**
   * @brief Doubles the current retransmission timeout when a timeout occurs.
   */
  void backoff_rto();

  /**
   * @brief Returns the current calculated Retransmission TimeOut.
   */
  std::chrono::milliseconds get_current_rto() const;

  void set_retransmit_deadline(std::chrono::steady_clock::time_point deadline);

  void clear_retransmit_deadline();

  int get_next_timeout_ms() const;

private:
  std::chrono::seconds global_timeout;
  std::chrono::steady_clock::time_point last_progress_time;

  // RFC 6298
  std::chrono::milliseconds srtt{0};
  std::chrono::milliseconds rttvar{0};

  // Initial RTO
  std::chrono::milliseconds rto{500};
  bool first_measurement = true;

  // Tracks when the Sender needs to wake up to retransmit a packet
  std::optional<std::chrono::steady_clock::time_point> retransmit_deadline;
};
