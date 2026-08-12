#ifndef PACKET_HPP
#define PACKET_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <stdexcept>

// Maximum allowed payload size (64 KB) to prevent memory allocation DoS attacks
constexpr uint32_t MAX_PAYLOAD_SIZE = 65536; 
constexpr uint8_t PROTOCOL_VERSION = 1;

// Packet Types
enum class PacketType : uint8_t {
    HELLO = 0x01,
    AUTH  = 0x02,
    DATA  = 0x03,
    PING  = 0x04,
    PONG  = 0x05,
    ERROR = 0x06
};

// Packed structure to prevent compiler padding bytes
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  version;  // 1 Byte
    uint8_t  type;     // 1 Byte (cast from PacketType)
    uint32_t length;   // 4 Bytes (Stored in Network Byte Order / Big-Endian)
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == 6, "PacketHeader size must be exactly 6 bytes!");

class Packet {
public:
    PacketHeader header;
    std::vector<uint8_t> payload;

    Packet() {
        header.version = PROTOCOL_VERSION;
        header.type = static_cast<uint8_t>(PacketType::DATA);
        header.length = 0;
    }

    Packet(PacketType type, const std::vector<uint8_t>& data) {
        if (data.size() > MAX_PAYLOAD_SIZE) {
            throw std::invalid_argument("Payload exceeds MAX_PAYLOAD_SIZE limit");
        }
        header.version = PROTOCOL_VERSION;
        header.type = static_cast<uint8_t>(type);
        header.length = htonl(static_cast<uint32_t>(data.size())); // Convert to Network Byte Order
        payload = data;
    }

    Packet(PacketType type, const std::string& str_data) 
        : Packet(type, std::vector<uint8_t>(str_data.begin(), str_data.end())) {}

    // Get payload size in host byte order
    uint32_t get_payload_length() const {
        return ntohl(header.length); // Convert from Network Byte Order to Host Order
    }

    PacketType get_type() const {
        return static_cast<PacketType>(header.type);
    }

    // Serialize packet into binary raw byte buffer for socket transmission
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer(sizeof(PacketHeader) + get_payload_length());
        
        // Copy 6-byte header
        std::memcpy(buffer.data(), &header, sizeof(PacketHeader));
        
        // Copy payload
        if (!payload.empty()) {
            std::memcpy(buffer.data() + sizeof(PacketHeader), payload.data(), payload.size());
        }

        return buffer;
    }

    // Deserialization & Malformed Packet Validation
    static bool deserialize(const uint8_t* buffer, size_t size, Packet& out_packet) {
        if (size < sizeof(PacketHeader)) {
            return false; // Incomplete header chunk
        }

        PacketHeader raw_hdr;
        std::memcpy(&raw_hdr, buffer, sizeof(PacketHeader));

        // 1. Validate Protocol Version
        if (raw_hdr.version != PROTOCOL_VERSION) {
            std::cerr << "[-] Protocol Error: Invalid Version (" << (int)raw_hdr.version << ")\n";
            return false;
        }

        // 2. Validate Payload Length
        uint32_t payload_len = ntohl(raw_hdr.length);
        if (payload_len > MAX_PAYLOAD_SIZE) {
            std::cerr << "[-] Protocol Error: Payload length " << payload_len << " exceeds MAX limit!\n";
            return false;
        }

        // 3. Ensure full packet payload is present in buffer
        if (size < sizeof(PacketHeader) + payload_len) {
            return false; // Partial packet received, need more bytes
        }

        out_packet.header = raw_hdr;
        out_packet.payload.assign(
            buffer + sizeof(PacketHeader),
            buffer + sizeof(PacketHeader) + payload_len
        );

        return true;
    }

    std::string payload_as_string() const {
        return std::string(payload.begin(), payload.end());
    }
};

#endif // PACKET_HPP
