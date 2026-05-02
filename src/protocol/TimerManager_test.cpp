#include "../../include/doctest.h"
#include "../../include/protocol/TimerManager.hpp"
#include <chrono>

using namespace std::chrono;

TEST_CASE("TimerManager basic functionality") {
  TimerManager tm(seconds(5));

  SUBCASE("Global timeout not expired initially") {
    CHECK_FALSE(tm.has_global_timeout_expired());
  }

  SUBCASE("Retransmit deadline management") {
    tm.clear_retransmit_deadline();
    auto now = steady_clock::now();
    tm.set_retransmit_deadline(now + milliseconds(100));

    int timeout = tm.get_next_timeout_ms();
    CHECK(timeout > 0);
    CHECK(timeout <= 100);

    tm.clear_retransmit_deadline();
    timeout = tm.get_next_timeout_ms();
    CHECK(timeout >= 4900); // Close to 5 seconds
  }

  SUBCASE("Expired retransmit deadline returns 0") {
    auto now = steady_clock::now();
    tm.set_retransmit_deadline(now - milliseconds(10));
    CHECK(tm.get_next_timeout_ms() == 0);
  }
}

TEST_CASE("TimerManager RTO calculation") {
  TimerManager tm(seconds(5));
  milliseconds initial_rto = tm.get_current_rto();

  tm.update_rtt(milliseconds(100));
  milliseconds rto1 = tm.get_current_rto();
  // RFC 6298: SRTT = 100, RTTVAR = 50, RTO = 100 + 4*50 = 300
  CHECK(rto1.count() == 300);

  tm.update_rtt(milliseconds(100));
  milliseconds rto2 = tm.get_current_rto();
  // Subsequent:
  // RTTVAR = 0.75 * 50 + 0.25 * |100 - 100| = 37.5 -> 37
  // SRTT = 0.875 * 100 + 0.125 * 100 = 100
  // RTO = 100 + 4 * 37 = 248
  CHECK(rto2.count() == 248);
}
