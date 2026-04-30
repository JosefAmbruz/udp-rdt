#include "../../include/doctest.h"
#include "../../include/net/Packet.hpp"
#include <vector>
#include <stdexcept>

TEST_CASE("Packet serialization and deserialization") {
    Packet pkt;
    pkt.connection_id = 0x12345678;
    pkt.seq_num = 100;
    pkt.ack_num = 50;
    pkt.flags = Packet::FLAG_SYN | Packet::FLAG_ACK;
    pkt.payload = {'H', 'e', 'l', 'l', 'o'};

    SUBCASE("Round-trip consistency") {
        auto serialized = pkt.serialize();
        Packet deserialized = Packet::deserialize(serialized);

        CHECK(deserialized.connection_id == pkt.connection_id);
        CHECK(deserialized.seq_num == pkt.seq_num);
        CHECK(deserialized.ack_num == pkt.ack_num);
        CHECK(deserialized.flags == pkt.flags);
        CHECK(deserialized.payload == pkt.payload);
    }

    SUBCASE("Corrupted packet detection") {
        auto serialized = pkt.serialize();
        // Corrupt one byte of the payload (at the end of the header)
        serialized[Packet::HEADER_SIZE] ^= 0xFF;
        
        CHECK_THROWS_AS(Packet::deserialize(serialized), std::invalid_argument);
    }

    SUBCASE("Truncated packet detection") {
        auto serialized = pkt.serialize();
        serialized.pop_back();
        
        CHECK_THROWS_AS(Packet::deserialize(serialized), std::invalid_argument);
    }
}
