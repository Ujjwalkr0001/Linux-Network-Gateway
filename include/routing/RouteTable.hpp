#ifndef ROUTE_TABLE_HPP
#define ROUTE_TABLE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <optional>
#include <cstdint>
#include <arpa/inet.h>

// Represents one routing rule: a destination CIDR + next-hop label
struct Route {
    uint32_t network;   // e.g. 192.168.1.0  (host byte order)
    uint32_t mask;      // e.g. 0xFFFFFF00   (host byte order, /24)
    int      prefix_len; // e.g. 24
    std::string next_hop; // e.g. "eth0" or "10.0.0.1"

    bool matches(uint32_t dest_ip) const {
        return (dest_ip & mask) == network;
    }
};

// -------------------------------------------------------------------------
// Binary Trie Node (1 child per bit direction: 0 or 1)
// -------------------------------------------------------------------------
struct TrieNode {
    std::array<std::unique_ptr<TrieNode>, 2> children{};
    std::optional<Route> route; // set only at a prefix endpoint
};

// -------------------------------------------------------------------------
// RoutingTable — implements Longest Prefix Match (LPM) via binary IP trie
// -------------------------------------------------------------------------
class RoutingTable {
public:
    RoutingTable() : root_(std::make_unique<TrieNode>()) {}

    // Insert CIDR string like "10.0.0.0/8" with a given next_hop
    bool add_route(const std::string& cidr, const std::string& next_hop) {
        std::string ip_str;
        int prefix_len = 32;
        auto slash = cidr.find('/');
        if (slash == std::string::npos) {
            ip_str = cidr;
        } else {
            ip_str = cidr.substr(0, slash);
            prefix_len = std::stoi(cidr.substr(slash + 1));
        }

        uint32_t network_ip = 0;
        if (inet_pton(AF_INET, ip_str.c_str(), &network_ip) != 1) {
            std::cerr << "[!] Invalid IP in route: " << cidr << "\n";
            return false;
        }
        network_ip = ntohl(network_ip); // convert to host byte order

        uint32_t mask = prefix_len == 0 ? 0 : (~0u << (32 - prefix_len));
        uint32_t network = network_ip & mask;

        Route r{ network, mask, prefix_len, next_hop };

        // Walk the trie, inserting exactly prefix_len bits
        TrieNode* node = root_.get();
        for (int bit = 31; bit >= (32 - prefix_len); --bit) {
            int direction = (network >> bit) & 1;
            if (!node->children[direction]) {
                node->children[direction] = std::make_unique<TrieNode>();
            }
            node = node->children[direction].get();
        }
        node->route = r;

        std::cout << "[+] Route Added: " << cidr << " -> " << next_hop << "\n";
        return true;
    }

    // Longest Prefix Match lookup for a destination IP string
    std::optional<Route> lookup(const std::string& dest_ip_str) const {
        uint32_t dest_ip = 0;
        if (inet_pton(AF_INET, dest_ip_str.c_str(), &dest_ip) != 1) {
            std::cerr << "[!] Invalid lookup IP: " << dest_ip_str << "\n";
            return std::nullopt;
        }
        dest_ip = ntohl(dest_ip);
        return lookup_ip(dest_ip);
    }

    std::optional<Route> lookup_ip(uint32_t dest_ip) const {
        TrieNode* node = root_.get();
        std::optional<Route> best_match;

        for (int bit = 31; bit >= 0 && node; --bit) {
            if (node->route.has_value()) {
                best_match = node->route; // longer prefix found so far
            }
            int direction = (dest_ip >> bit) & 1;
            node = node->children[direction].get();
        }
        // Check leaf node too
        if (node && node->route.has_value()) {
            best_match = node->route;
        }
        return best_match;
    }

private:
    std::unique_ptr<TrieNode> root_;
};

#endif // ROUTE_TABLE_HPP
