#include "../../include/protocol/TimerManager.hpp"
#include "../../include/dbg.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>

using namespace std::chrono;

TimerManager::TimerManager(seconds global_timeout)
    : global_timeout(global_timeout), last_progress_time(steady_clock::now()) {}

void TimerManager::register_progress() {
  dbg("PROGRESS REGISTERED");
  last_progress_time = steady_clock::now();
}

bool TimerManager::has_global_timeout_expired() const {
  auto now = steady_clock::now();
  return duration_cast<seconds>(now - last_progress_time) >= global_timeout;
}

void TimerManager::update_rtt(milliseconds measured_rtt) {
  if (first_measurement) {
    /**
     * (2.2) When the first RTT measurement R is made, the host MUST set
     *        SRTT <- R
     *        RTTVAR <- R/2
     *        RTO <- SRTT + max (G, K*RTTVAR) <--- This part is same.
     *     where K = 4.
     */
    srtt = measured_rtt;
    rttvar = measured_rtt / 2;
    first_measurement = false;
  } else {
    /**
     * (2.3) When a subsequent RTT measurement R' is made, a host MUST set
     *        RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
     *        SRTT <- (1 - alpha) * SRTT + alpha * R'
     *
     *   The above SHOULD be computed using alpha=1/8 and beta=1/4 (as
     *   suggested in [JK88]).
     */
    auto diff = std::abs(srtt.count() - measured_rtt.count());
    rttvar = milliseconds(
        static_cast<long long>(0.75 * rttvar.count() + 0.25 * diff));

    srtt = milliseconds(static_cast<long long>(0.875 * srtt.count() +
                                               0.125 * measured_rtt.count()));
  }

  // RTO <- SRTT + max (G, K*RTTVAR)
  rto = srtt + 4 * rttvar;

  // Bounds for the RTO
  // RFC suggests <1;60> second interval, but I feel 1s is too large for the
  // assignment
  rto = std::max(rto, milliseconds(10));
  rto = std::min(rto, milliseconds(60000));
}

// When the retransmission timer expires, RFC suggests backing off the timer
// RFC 6298 step 5.5
void TimerManager::backoff_rto() {
  // Max bound is still 60 seconds
  rto = std::min(rto * 2, milliseconds(60000));
}

milliseconds TimerManager::get_current_rto() const { return rto; }

void TimerManager::set_retransmit_deadline(steady_clock::time_point deadline) {
  retransmit_deadline = deadline;
}

void TimerManager::clear_retransmit_deadline() {
  retransmit_deadline = std::nullopt;
}

int TimerManager::get_next_timeout_ms() const {
  auto now = steady_clock::now();

  auto global_timeout_expiration = last_progress_time + global_timeout;
  auto time_to_global =
      duration_cast<milliseconds>(global_timeout_expiration - now).count();

  if (time_to_global <= 0) {
    return 0; // Global timeout already expired
  }

  // Calculate time until next retransmission, if one is scheduled
  if (retransmit_deadline.has_value()) {
    auto time_to_rto =
        duration_cast<milliseconds>(retransmit_deadline.value() - now).count();
    if (time_to_rto <= 0) {
      return 0; // RTO has already expired, return immediately to trigger rt.
    }

    // Return global_timeout or rto, based on what happens first
    return static_cast<int>(std::min(time_to_global, time_to_rto));
  }

  // If no packets are out there, return global timeout
  return static_cast<int>(time_to_global);
}
