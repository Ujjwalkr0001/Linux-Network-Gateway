#ifndef PACKET_DISSECTOR_HPP
#define PACKET_DISSECTOR_HPP

// ─────────────────────────────────────────────────────────────────────────
// PACKET CAPTURE & DISSECTION SUBSYSTEM
//
// Layer 2, Layer 3, Layer 4 Protocol Demuxing & Header Parsing:
//   - Layer 2: Ethernet II (MAC Addrs, EtherType)
//   - Layer 3: IPv4 (IP Version, IHL, Total Len, Protocol, Source/Dest IP)
//   - Layer 4: TCP (Ports, Seq, Ack, Flags: SYN, ACK, FIN, RST, PSH)
//   - Layer 4: UDP (Ports, Length, Checksum)
// ─────────────────────────────────────────────────────────────────────────

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

#pragma pack(push, 1)

// Layer 2 Ethernet II Header (14 bytes)
struct EthernetHeader {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t ether_type; // e.g. 0x0800 (IPv4), 0x0806 (ARP)
};

// Layer 3 IPv4 Header (20 bytes minimum)
struct IPv4Header {
    uint8_t  ver_ihl;       // Version (4 bits) + IHL (4 bits)
    uint8_t  tos;           // Type of Service
    uint16_t total_length;  // Total length (bytes)
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t  ttl;           // Time to Live
    uint8_t  protocol;      // Protocol (6 = TCP, 17 = UDP, 1 = ICMP)
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
};

// Layer 4 TCP Header (20 bytes minimum)
struct TCPHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset; // Data offset (4 bits) + Reserved (4 bits)
    uint8_t  flags;       // URG, ACK, PSH, RST, SYN, FIN
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
};

// Layer 4 UDP Header (8 bytes)
struct UDPHeader {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
};

#pragma pack(pop)

static_assert(sizeof(EthernetHeader) == 14, "EthernetHeader size must be 14 bytes!");
static_assert(sizeof(IPv4Header) == 20, "IPv4Header size must be 20 bytes!");
static_assert(sizeof(UDPHeader) == 8, "UDPHeader size must be 8 bytes!");

class PacketDissector {
public:
    static std::string mac_to_string(const uint8_t mac[6]) {
        std::ostringstream oss;
        for (int i = 0; i < 6; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)mac[i];
            if (i < 5) oss << ":";
        }
        return oss.str();
    }

    static std::string ip_to_string(uint32_t ip_net) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_net, buf, sizeof(buf));
        return std::string(buf);
    }

    static void dissect_frame(const uint8_t* buffer, size_t size) {
        std::cout << "====================================================================================================\n";
        std::cout << " RAW PACKET FRAME DISSECTION [" << size << " Bytes]\n";
        std::cout << "====================================================================================================\n";

        if (size < sizeof(EthernetHeader)) {
            std::cout << "[!] Frame truncated: less than 14-byte Ethernet header\n";
            return;
        }

        // 1. Layer 2 Ethernet II Dissection
        EthernetHeader eth;
        std::memcpy(&eth, buffer, sizeof(EthernetHeader));
        uint16_t ether_type = ntohs(eth.ether_type);

        std::cout << "[L2 Ethernet II]  Src MAC: " << mac_to_string(eth.src_mac)
                  << "  ->  Dst MAC: " << mac_to_string(eth.dest_mac)
                  << "  [EtherType: 0x" << std::hex << std::setw(4) << std::setfill('0') << ether_type << std::dec << "]\n";

        if (ether_type != 0x0800) {
            std::cout << "  (Non-IPv4 Frame: " << (ether_type == 0x0806 ? "ARP" : "Other") << ")\n";
            return;
        }

        // 2. Layer 3 IPv4 Dissection
        size_t offset = sizeof(EthernetHeader);
        if (size < offset + sizeof(IPv4Header)) {
            std::cout << "[!] Truncated IPv4 Packet\n";
            return;
        }

        IPv4Header ip;
        std::memcpy(&ip, buffer + offset, sizeof(IPv4Header));
        uint8_t ihl = (ip.ver_ihl & 0x0F) * 4;
        uint16_t total_len = ntohs(ip.total_length);

        std::cout << "[L3 IPv4]         Src IP : " << ip_to_string(ip.src_ip)
                  << "  ->  Dst IP : " << ip_to_string(ip.dest_ip)
                  << "  [TTL: " << (int)ip.ttl << ", Total Len: " << total_len << " B, Protocol: " << (int)ip.protocol << "]\n";

        offset += ihl;

        // 3. Layer 4 Dissection (TCP / UDP)
        if (ip.protocol == 6) { // TCP
            if (size < offset + sizeof(TCPHeader)) {
                std::cout << "[!] Truncated TCP Segment\n";
                return;
            }
            TCPHeader tcp;
            std::memcpy(&tcp, buffer + offset, sizeof(TCPHeader));

            std::cout << "[L4 TCP]          Src Port: " << ntohs(tcp.src_port)
                      << "  ->  Dst Port: " << ntohs(tcp.dest_port)
                      << "  [Seq: " << ntohl(tcp.seq_num) << ", Ack: " << ntohl(tcp.ack_num) << "]";

            std::cout << "  Flags: [ ";
            if (tcp.flags & 0x02) std::cout << "SYN ";
            if (tcp.flags & 0x10) std::cout << "ACK ";
            if (tcp.flags & 0x01) std::cout << "FIN ";
            if (tcp.flags & 0x04) std::cout << "RST ";
            if (tcp.flags & 0x08) std::cout << "PSH ";
            std::cout << "]\n";

        } else if (ip.protocol == 17) { // UDP
            if (size < offset + sizeof(UDPHeader)) {
                std::cout << "[!] Truncated UDP Segment\n";
                return;
            }
            UDPHeader udp;
            std::memcpy(&udp, buffer + offset, sizeof(UDPHeader));

            std::cout << "[L4 UDP]          Src Port: " << ntohs(udp.src_port)
                      << "  ->  Dst Port: " << ntohs(udp.dest_port)
                      << "  [Length: " << ntohs(udp.length) << " B]\n";
        }
        std::cout << "====================================================================================================\n\n";
    }
};

#endif // PACKET_DISSECTOR_HPP
