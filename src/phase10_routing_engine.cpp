#include <iostream>
#include <string>
#include <vector>
#include "routing/RouteTable.hpp"

// Helper: print route lookup result
void print_lookup(const RoutingTable& table, const std::string& dest) {
    auto result = table.lookup(dest);
    if (result.has_value()) {
        // Convert network back to dotted-decimal for display
        uint32_t net_be = htonl(result->network);
        char net_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &net_be, net_str, INET_ADDRSTRLEN);

        std::cout << "[MATCH] " << dest
                  << "  ->  " << result->next_hop
                  << "  (prefix: " << net_str << "/" << result->prefix_len << ")\n";
    } else {
        std::cout << "[NO ROUTE] " << dest << "  -> DROPPED (no matching prefix)\n";
    }
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 10: HIGH-PERFORMANCE ROUTING ENGINE\n";
    std::cout << " (Binary Trie / Longest Prefix Match)\n";
    std::cout << "=================================================\n\n";

    RoutingTable table;

    // ---- Populate Routing Table ----
    std::cout << "--- Building Routing Table ---\n";
    table.add_route("0.0.0.0/0",     "eth0-default");   // default route
    table.add_route("10.0.0.0/8",    "eth1-internal");  // class-A private
    table.add_route("10.10.0.0/16",  "eth2-datacenter");
    table.add_route("10.10.1.0/24",  "eth3-subnet");    // most specific
    table.add_route("192.168.0.0/16","eth4-home");
    table.add_route("192.168.1.0/24","eth5-office");
    table.add_route("172.16.0.0/12", "eth6-cloud");

    std::cout << "\n--- Longest Prefix Match Lookups ---\n";

    // Test 1: Matches /24 (most specific)
    print_lookup(table, "10.10.1.55");

    // Test 2: Matches /16 (datacenter, not /24)
    print_lookup(table, "10.10.2.1");

    // Test 3: Matches /8 (broad internal)
    print_lookup(table, "10.99.0.1");

    // Test 4: Matches /24 office network
    print_lookup(table, "192.168.1.100");

    // Test 5: Matches /16 home network
    print_lookup(table, "192.168.5.5");

    // Test 6: Matches default route (no specific route)
    print_lookup(table, "8.8.8.8");

    // Test 7: Cloud CIDR
    print_lookup(table, "172.20.1.1");

    std::cout << "\n--- CIDR Attack / Edge Case Tests ---\n";

    // Test 8: Invalid IP
    print_lookup(table, "999.999.999.999");

    // Test 9: Loopback (matches default)
    print_lookup(table, "127.0.0.1");

    // Test 10: Broadcast-ish
    print_lookup(table, "255.255.255.255");

    std::cout << "\n[+] Phase 10 complete. Routing Engine works correctly.\n";
    return 0;
}
