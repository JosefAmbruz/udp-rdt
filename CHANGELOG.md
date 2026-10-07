# Changelog
All notable changes to the udp-rdt project are documented in this file.

## [1.0.0] - 2026-05-03

### Added
- **Reliable Transport**: Implementation of Selective Repeat (SR) ARQ strategy with segment sequencing.
- **Congestion Handling**: Dynamic RTO calculation based on RFC 6298 (SRTT/RTTVAR) with exponential backoff.
- **Session Management**: Robust 3-way handshake and 4-way teardown state machines.
- **Data Integrity**: IEEE 802.3 CRC32 checksum protection across headers and payload.
- **Session Protection**: 32-bit cryptographically random Connection ID to prevent session confusion and crosstalk.
- **Flow Control & Backpressure**: Sliding window (size 64) integrated with `poll()`-based I/O backpressure.
- **Dual-Stack Protocol Support**: Full IPv4 and IPv6 support via POSIX sockets.
- **Flexible I/O Streams**: Support for arbitrary files, standard input (`stdin`), and standard output (`stdout`).
- **Automated Verification**:
  - Unit tests for CRC32 and timer math (`doctest`).
  - Integration tests for stream modes and multi-megabyte transfers (`unittest`).
  - Network impairment testing via custom Python proxy (simulating loss, corruption, delay, jitter, and reordering).
  - Linux kernel-level resilience tests using `tc netem`.
  - Memory safety verification under Valgrind Memcheck.

### Known Limitations
- **Fixed Window Size**: The implementation uses a constant window size of 64 segments without dynamic congestion window resizing (e.g., AIMD/CUBIC).
- **Single Session per Execution**: The receiver endpoint terminates after completing a single transfer session.
- **Initial Handshake RTO**: The initial SYN transmission uses a fixed 500ms timeout baseline before empirical RTT measurements are sampled.
