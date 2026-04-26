#include "../../include/net/Packet.hpp"

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/types.h>
#include <vector>

bool Packet::has_flag(uint16_t flag) const {
  // 1101 & 0010 -> 0; 1111 & 0010 -> 0010 != 0
  return (flags & flag) != 0;
}

void Packet::set_flag(uint16_t flag) {
  // 1101 | 0010 -> 1111
  flags |= flag;
}

std::vector<uint8_t> Packet::serialize() const {
  if (payload.size() > MAX_PAYLOAD_SIZE) {
    throw std::invalid_argument("Packet payload exceeds MAX_PAYLOAD_SIZE");
  }

  std::vector<uint8_t> buffer(HEADER_SIZE + payload.size());

  // Convert to network byte order
  uint32_t net_conn_id = htonl(connection_id);
  uint32_t net_seq_num = htonl(seq_num);
  uint32_t net_ack_num = htonl(ack_num);
  uint16_t net_flags = htons(flags);
  uint16_t net_payload_len = htons(static_cast<uint16_t>(payload.size()));

  // Pack data into the byte array
  std::memcpy(buffer.data() + 0, &net_conn_id, 4);
  std::memcpy(buffer.data() + 4, &net_seq_num, 4);
  std::memcpy(buffer.data() + 8, &net_seq_num, 4);
  std::memcpy(buffer.data() + 12, &net_flags, 2);
  std::memcpy(buffer.data() + 14, &net_payload_len, 2);

  // Set checksum placeholder
  uint32_t zero_checksum = 0;
  std::memcpy(buffer.data() + 16, &zero_checksum, 4);

  // Append payload
  if (!payload.empty()) {
    std::memcpy(buffer.data() + HEADER_SIZE, payload.data(), payload.size());
  }

  // Calculate the actual checksum
  uint32_t checksum = calculate_crc32(buffer);
  uint32_t net_checksum = htonl(checksum); // Net byte order
  std::memcpy(buffer.data() + 16, &net_checksum, 4);

  return buffer;
}

Packet Packet::deserialize(const std::vector<uint8_t> &data) {
  if (data.size() < HEADER_SIZE) {
    throw std::invalid_argument("Packet is smaller than the header size");
  }

  // Verify the checksum

  /// ...

  Packet pkt;

  return pkt;
}
