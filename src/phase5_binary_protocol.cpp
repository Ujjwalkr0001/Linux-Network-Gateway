#include <iostream>
#include <vector>
#include <iomanip>
#include "protocol/Packet.hpp"

void print_hex(const std::vector<uint8_t>& bytes) {
    std::cout << "Hex Dump [" << bytes.size() << " bytes]: ";
    for (uint8_t b : bytes) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
    }
    std::cout << std::dec << "\n";
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 5: CUSTOM BINARY APPLICATION PROTOCOL\n";
    std::cout << "=================================================\n";

    // 1. Create a HELLO Packet
    Packet hello_pkt(PacketType::HELLO, "Gateway Client v1.0");
    std::vector<uint8_t> serialized_hello = hello_pkt.serialize();

    std::cout << "[+] Serialized HELLO Packet:\n";
    std::cout << "  - Version: " << (int)hello_pkt.header.version << "\n";
    std::cout << "  - Type: 0x01 (HELLO)\n";
    std::cout << "  - Length (Host Order): " << hello_pkt.get_payload_length() << " bytes\n";
    print_hex(serialized_hello);

    // 2. Deserialize back
    Packet parsed_hello;
    if (Packet::deserialize(serialized_hello.data(), serialized_hello.size(), parsed_hello)) {
        std::cout << "[✓] Successfully Deserialized Packet!\n";
        std::cout << "  - Type: " << (int)parsed_hello.get_type() << "\n";
        std::cout << "  - Payload String: " << parsed_hello.payload_as_string() << "\n\n";
    }

    // 3. Create a DATA Packet
    Packet data_pkt(PacketType::DATA, "Qualcomm High-Performance Gateway Payload");
    std::vector<uint8_t> serialized_data = data_pkt.serialize();

    std::cout << "[+] Serialized DATA Packet:\n";
    print_hex(serialized_data);

    // 4. Test Malformed Packet Detection (Corrupted Header Length)
    std::cout << "\n[+] Testing Malformed Packet Detection (Corrupt Length Attack)...\n";
    std::vector<uint8_t> corrupt_packet = serialized_data;
    
    // Corrupt length field to massive fake size (e.g. 100,000 bytes)
    uint32_t fake_length = htonl(100000);
    std::memcpy(corrupt_packet.data() + 2, &fake_length, sizeof(uint32_t));

    Packet out_pkt;
    if (!Packet::deserialize(corrupt_packet.data(), corrupt_packet.size(), out_pkt)) {
        std::cout << "[✓] Successfully Rejected Malformed Packet! Protocol Security Intact.\n";
    }

    std::cout << "\n=================================================\n";
    std::cout << " PHASE 5 COMPLETE!\n";
    std::cout << "=================================================\n";
    return 0;
}
;;;;;;;