# Changelog
All notable changes to the ipk-rdt project are documented in this file.

[1.0.0] - 2026-05-03

## Added
- Reliable Transport: Implementation of Selective Repeat (SR) ARQ strategy.
- Congestion Handling: Dynamic RTO calculation based on RFC 6298 (SRTT/RTTVAR).
- Session Management: Robust 3-way handshake and 4-step teardown procedures.
- Data Integrity: IEEE 802.3 CRC32 checksum protection for headers and payload.
- Session Protection: 32-bit Connection ID to prevent session confusion.
- Flow Control: Sliding window (size 64) with poll()-based backpressure.
- Protocol Support: Full dual-stack support for IPv4 and IPv6.
- Flexible I/O: Support for files, stdin, and stdout as transfer sources/sinks.
- Automated Testing:
    - Unit tests for protocol logic (doctest).
    - Integration tests for I/O and large file transfers (unittest).
    - Resilience tests with a custom UDP proxy for loss/jitter simulation.
    - OS-level resilience tests using tc netem.
    - Memory safety verification using valgrind.

## Known Limitations
- Fixed Window Size: The implementation uses a constant window size of 64 segments and does not implement advanced congestion control algorithms (like TCP Cubic or NewReno).
- Single Client: The server is designed to handle exactly one transfer per execution, as per the assignment specification.
- Initial Handshake RTO: The very first SYN packet uses a fixed 500ms timeout before SRTT measurements are available.`RdtReceiver`.
