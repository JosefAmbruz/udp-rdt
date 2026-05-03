# ipk-rdt: Reliable Data Transfer over UDP

## Project Overview
`ipk-rdt` is a user-space transport protocol implementation that provides reliable, ordered, and integrity-protected delivery of arbitrary byte streams over the unreliable UDP protocol. It is designed to emulate the core reliability features of TCP while adhering to specific project constraints, such as a 1200-byte maximum segment size.

### Key Features:
  * **Reliability**: Implements the *Selective Repeat* strategy to handle packet loss efficiently without unnecessary retransmissions of the entire window.
  * **Adaptive Timing**: Uses the *RFC 6298* algorithm to calculate a dynamic Retransmission Timeout based on real-time Round Trip Time measurements.
  * **Connection Management**: Employs a robust 3-way handshake for session establishment and a 4-step teardown for graceful termination.
  * **Integrity**: Every packet is protected by an *IEEE 802.3 CRC32* checksum covering both the 20-byte header and the payload.
  * **Flow Control**: Implements a sliding window of 64 segments with backoff mechanisms to handle local I/O bottlenecks.
  * **Network Support**: Full dual-stack support for IPv4 and IPv6 addressing.

## Build and Run

### Prerequisites
  * **Environment**: Linux (x86_64) with `g++` supporting `C++20`.
  * **Tools**: `make`, `g++`, `awk`, `dd`.
  * **Testing Dependencies**: `Python 3.10+`, `valgrind`, `iproute2` (for tc netem).

### Compilation
To build the primary executable, run the following command in the project root:

```bash
make
```

This produces the standalone binary `ipk-rdt`.

### Basic Usage
The application operates in either *server* (-s) or *client* (-c) mode.

**Start the Server**:

``` bash
./ipk-rdt -s -p 9000 -o received_data.bin
-p: UDP port to listen on.
-o: Destination file (omitting this or using - defaults to stdout).
```

**Start the Client**:

``` bash
./ipk-rdt -c -a 127.0.0.1 -p 9000 -i source_data.bin
-a: Destination IPv4/IPv6 address or hostname.
-i: Source file (omitting this or using - defaults to stdin).
```

**Cleanup**
To remove compiled object files, binaries, and temporary test artifacts:

``` bash
make clean
```

**Execution Examples**
Stdin to Stdout Transfer:

``` bash
# Terminal 1 (Server)
./ipk-rdt -s -p 9000

# Terminal 2 (Client)
echo "Hello IPK" | ./ipk-rdt -c -a 127.0.0.1 -p 9000
```

IPv6 Transfer with Timeout:

``` bash
./ipk-rdt -s -p 9000 -a ::1 -w 5
./ipk-rdt -c -a ::1 -p 9000 -i large_file.zip -w 5
```

## Protocol Specification


... diagrams here

sequence_diagram
  participant C as Client (Sender)
  participant S as Server (Receiver)

  Note over C,S: 3-Way Handshake
  C->>S: SYN (seq=0)
  S->>C: SYN-ACK (seq=0, ack=0, conn_id=123)
  C->>S: ACK (ack=0, conn_id=123)

  Note over C,S: Data Transfer (Selective Repeat)
  C->>S: Data (seq=1, payload=...)
  S->>C: ACK (ack=1)

  Note right of S: Packet 2 is lost
  C-xS: Data (seq=2)
  C->>S: Data (seq=3)
  S->>C: ACK (ack=3)

  Note left of C: Timer expires for seq=2
  C->>S: Retransmit Data (seq=2)
  S->>C: ACK (ack=2)

  Note over C,S: Teardown
  C->>S: FIN (seq=4)
  S->>C: ACK (ack=4)
  S->>C: FIN (seq=X)
  C->>S: ACK (ack=X)


## Implementation Design


## Testing and Performance


## Known Limitations
* Fixed window size
* Handshake uses fixed 500ms initial RTO

## References

