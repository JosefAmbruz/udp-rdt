#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

/**
 * @brief Represents a single protocol data unit for the custom protocol.
 * Handles the 20 byte header, CRC32 checksum and serialization.
 */

class Packet {
public:
  static constexpr size_t HEADER_SIZE = 20;
  static constexpr size_t MAX_PAYLOAD_SIZE = 1180; // 1200 - 20

  // Flag Bitmask
  static constexpr uint16_t FLAG_SYN = 1 << 0;
  static constexpr uint16_t FLAG_ACK = 1 << 1;
  static constexpr uint16_t FLAG_FIN = 1 << 2;
  static constexpr uint16_t FLAG_RST = 1 << 3;

  // Packet data
  uint32_t connection_id = 0;
  uint32_t seq_num = 0;
  uint32_t ack_num = 0;
  uint16_t flags = 0;
  std::vector<uint8_t> payload;

  /**
   * @brief Default constructor for constructing outgoing packets
   */
  Packet() = default;

  /**
   * @brief Serializes the packet into byte array ready to be sent.
   * Computes the CRC32 checksum over the header and the payload.
   * @return Raw byte vector
   * @throws std::invalid_argument if the payload exceeds the MAX_PAYLOAD_SIZE.
   */
  std::vector<uint8_t> serialize() const;

  /**
   * @brief Deserializes a byte array into a structured Packet object
   *
   *
   */
  static Packet deserialize(const std::vector<uint8_t> &data);

  // Helper methods
  bool has_flag(uint16_t flag) const;

  void set_flag(uint16_t flag);

private:
  static uint32_t calculate_crc32(const std::vector<uint8_t> &data);
};
