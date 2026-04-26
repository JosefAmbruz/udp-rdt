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
  std::memcpy(buffer.data() + 8, &net_ack_num, 4);
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
  std::vector<uint8_t> verify_buffer = data;
  uint32_t zero_checksum = 0;
  std::memcpy(verify_buffer.data() + 16, &zero_checksum, 4);

  uint32_t calculated_checksum = calculate_crc32(verify_buffer);

  uint32_t received_net_checksum;
  std::memcpy(&received_net_checksum, data.data() + 16, 4);
  uint32_t received_checksum = ntohl(received_net_checksum);

  if (calculated_checksum != received_checksum) {
    throw std::invalid_argument(
        "Corrupted packet, CRC32 checksum verification failed.");
  }

  // Extract fields
  Packet pkt;
  uint32_t net_conn_id, net_seq_num, net_ack_num;
  uint16_t net_flags, net_payload_len;

  std::memcpy(&net_conn_id, data.data() + 0, 4);
  std::memcpy(&net_seq_num, data.data() + 4, 4);
  std::memcpy(&net_ack_num, data.data() + 8, 4);
  std::memcpy(&net_flags, data.data() + 12, 2);
  std::memcpy(&net_payload_len, data.data() + 14, 2);

  pkt.connection_id = ntohl(net_conn_id);
  pkt.seq_num = ntohl(net_seq_num);
  pkt.ack_num = ntohl(net_ack_num);
  pkt.flags = ntohs(net_flags);
  uint16_t payload_len = ntohs(net_payload_len);

  // Validate payload size
  if (data.size() < HEADER_SIZE + payload_len) {
    throw std::invalid_argument("Expected payload size exceeds the received "
                                "bytes. Packet was truncated.");
  }

  // Extract payload
  if (payload_len > 0) {
    pkt.payload.assign(data.begin() + HEADER_SIZE,
                       data.begin() + HEADER_SIZE + payload_len);
  }

  return pkt;
}

uint32_t Packet::calculate_crc32(const std::vector<uint8_t> &data) {
  uint32_t crc = 0xFFFFFFFF;
  // TODO: standard crc32 calculation. I should replace this
  // with optimal implementation from zlib.h

  // IEEE 802.3 standard bitwise CRC32
  // (Poly: 0xEDB88320)
  for (uint8_t byte : data) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
  }

  return ~crc;
}
