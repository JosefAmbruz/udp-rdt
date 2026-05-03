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

### Packet header format
The protocol uses a fixed size 20 byte header that provides all the necessary control information with minimal overhead. All multi byte fields are transmitted in Network Byte Order.

| Byte Offset | Field Name            | Size (Bytes) | Description                                           |
|:------------|:----------------------|:-------------|:------------------------------------------------------|
| 0           | Connection ID         | 4            | Unique 32-bit identifier for the session.             |
| 4           | Sequence Number       | 4            | Increasing ID for data segments.                      |
| 8           | Acknowledgment Number | 4            | The sequence number being confirmed by the peer.      |
| 12          | Flags                 | 2            | Control bits: SYN(0x1), ACK(0x2), FIN(0x4), RST(0x8). |
| 14          | Payload Length        | 2            | Size of the data following the header (0 to 1180).    |
| 16          | Checksum              | 4            | IEEE 802.3 CRC32 over the header and payload.         |

### Connection Identification
To prevent accidental confusion between packets from different transfers or late-arriving segments from a previous session, a 32-bit Connection ID is used. During the Handshake, the Receiver generates a cryptographically secure random 32-bit integer upon receiving a SYN packet. This ID is sent back in the SYN-ACK. From that point forward, both the Sender and Receiver discard any packets that do not contain the matching Connection ID.

### Session Management
**Handshake**
  1. `SYN`: Sender sends a `SYN` (seq=0) and enters SYN_SENT state.
  2. `SYN-ACK`: Receiver responds with `SYN-ACK` (seq=0, ack=0, ConnID) and enters SYN_RCVD.
  3. `ACK`: Sender receives the ID, sends a final `ACK`, and enters ESTABLISHED. The Receiver enters ESTABLISHED upon arrival of the first `ACK` or data packet.

**Teardown**
  1. `FIN`: Once the Sender reaches EOF and all data is `ACK`ed, it sends a `FIN` packet.
  2. `ACK`: Receiver acknowledges the `FIN`.
  3. `FIN`: Receiver sends its own `FIN` to signal it is ready to close.
  4. `ACK`: Sender sends the final `ACK` and enters CLOSED. Receiver enters CLOSED upon receipt.


```mermaid
sequenceDiagram
  participant C as Client (Sender)
  participant S as Server (Receiver)

  Note over C,S: 3-Way Handshake
  C->>S: SYN (seq=0)
  S->>C: SYN-ACK (seq=0, ack=0, conn_id=123)
  C->>S: ACK (ack=0, conn_id=123)

  Note over C,S: Data Transfer (Selective Repeat)
  C->>S: Data (seq=1, conn_id=123, payload=...)
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
```

### Reliability and Window Management
**Sequencing**
The protocol uses segment-based sequencing rather than byte-offsets. Each data segment (up to 1180 bytes) is assigned a single 32-bit sequence number. This simplifies the management of the sliding window and out-of-order buffers.

**Selective Repeat**
* Sender: Maintains a `std::deque` of unacknowledged packets. When an ACK arrives, the specific packet is marked as is_acked. The window only "slides" (removes packets) when the packet at the front of the deque is marked as ACKed.
* Receiver: Uses a `std::map<uint32_t, vector<uint8_t>>` to buffer packets that arrive out-of-order. When the hole is filled (i.e., pkt.seq_num == rcv_base), the Receiver flushes all contiguous packets from the buffer to the output stream.

**Retransmission Strategy (RFC 6298)**
The protocol uses a dynamic Retransmission TimeOut (RTO) to adapt to network conditions:
* Initial RTO: 500ms.
* Calculations:
    * $RTTVAR = (0.75 \times RTTVAR) + (0.25 \times |SRTT - R|)$
    * $SRTT = (0.875 \times SRTT) + (0.125 \times R)$
    * $RTO = SRTT + \max(10ms, 4 \times RTTVAR)$
* Exponential Backoff: If a timeout occurs, the RTO is doubled ($RTO = RTO \times 2$) up to a maximum of 5 seconds to ensure it stays within the typical -w global timeout limits.

**Window Behavior**
* Fixed Window Size: 64 segments.
* Segment Limit: 1180 bytes of payload per UDP datagram.
* Backpressure: The Sender uses the get_io_fd() mechanism to stop reading from the input file when the window is full (64 segments), resuming only when an ACK slides the window forward.

## Implementation Design


## Testing and Performance


## Known Limitations
* Fixed window size
* Handshake uses fixed 500ms initial RTO

## References

