# ipk-rdt

A simplified transport protocol inspired by TCP, implemented in user space above UDP.

## Testing Strategy

The project uses a two-tier automated testing approach to ensure both internal logic correctness and end-to-end protocol resilience.

### 1. Unit Testing
Unit tests are implemented using the [doctest](https://github.com/doctest/doctest) framework. These tests focus on individual components:
- **Packet handling:** Serialization, deserialization, and CRC32 verification.
- **Timer Management:** Verification of retransmission triggers and timer accuracy.
- **Protocol Logic:** Isolated testing of window management (GBN/SR) and ACK processing.

Run unit tests with:
```bash
make test
```

### 2. Integration Testing
System-level tests are implemented in Python to verify the protocol's behavior under real-world network conditions:
- **Data Integrity:** Verification of byte-for-byte identical transfers using SHA-256 hashes.
- **Network Resilience:** Simulation of packet loss, reordering, and corruption using `tc netem` or internal simulation.
- **CLI Compliance:** Automated verification of all mandatory command-line arguments and I/O modes (files, stdin/stdout).
- **Timeout Behavior:** Verification of the `-w` timeout implementation.

The integration tests are automatically executed as part of the `make test` target.