#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <thread>

#include "protocol/Packet.hpp"
#include "routing/RouteTable.hpp"
#include "ratelimit/RateLimiter.hpp"
#include "auth/AuthSession.hpp"

using namespace std::chrono_literals;

// ── Lightweight Unit Test Harness ─────────────────────────────────────────

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            g_tests_passed++; \
        } else { \
            g_tests_failed++; \
            std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__ << " - " << msg << "\n"; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(val1, val2, msg) TEST_ASSERT((val1) == (val2), msg)
#define TEST_ASSERT_TRUE(cond, msg)     TEST_ASSERT((cond) == true, msg)
#define TEST_ASSERT_FALSE(cond, msg)    TEST_ASSERT((cond) == false, msg)

// ── Test Suite 1: Binary Application Protocol ────────────────────────────
void test_suite_binary_protocol() {
    std::cout << "--- Test Suite 1: Binary Protocol Parser (Packet.hpp) ---\n";

    // 1. Valid Packet Creation & Serialization
    Packet orig_pkt(PacketType::DATA, "Qualcomm Unit Test Payload");
    std::vector<uint8_t> raw = orig_pkt.serialize();

    TEST_ASSERT_TRUE(raw.size() == sizeof(PacketHeader) + orig_pkt.get_payload_length(), "Serialized byte length check");

    // 2. Deserialization
    Packet parsed_pkt;
    bool ok = Packet::deserialize(raw.data(), raw.size(), parsed_pkt);
    TEST_ASSERT_TRUE(ok, "Deserialization succeeds for valid packet");
    TEST_ASSERT_EQ(parsed_pkt.header.version, PROTOCOL_VERSION, "Protocol version matches");
    TEST_ASSERT_TRUE(parsed_pkt.get_type() == PacketType::DATA, "Packet type matches");
    TEST_ASSERT_EQ(parsed_pkt.payload_as_string(), "Qualcomm Unit Test Payload", "Payload contents match");

    // 3. Invalid Version Rejection
    raw[0] = 99; // Corrupt version byte
    TEST_ASSERT_FALSE(Packet::deserialize(raw.data(), raw.size(), parsed_pkt), "Rejects corrupted version byte");

    // 4. Truncated Header Rejection
    uint8_t short_buf[3] = { 0x01, 0x01, 0x00 };
    TEST_ASSERT_FALSE(Packet::deserialize(short_buf, sizeof(short_buf), parsed_pkt), "Rejects truncated buffer (< 6 bytes)");
}

// ── Test Suite 2: High-Performance Routing Engine ───────────────────────
void test_suite_routing_engine() {
    std::cout << "--- Test Suite 2: Routing Engine Binary Trie (RouteTable.hpp) ---\n";

    RoutingTable table;
    table.add_route("0.0.0.0/0",     "eth0-default");
    table.add_route("10.0.0.0/8",    "eth1-broad");
    table.add_route("10.10.0.0/16",  "eth2-datacenter");
    table.add_route("10.10.1.0/24",  "eth3-subnet");

    // 1. Longest Prefix Match (/24 subnet match)
    auto res1 = table.lookup("10.10.1.50");
    TEST_ASSERT_TRUE(res1.has_value(), "Lookup 10.10.1.50 finds match");
    TEST_ASSERT_EQ(res1->next_hop, "eth3-subnet", "LPM matches most specific /24 prefix");

    // 2. Medium Prefix Match (/16 match)
    auto res2 = table.lookup("10.10.2.1");
    TEST_ASSERT_TRUE(res2.has_value(), "Lookup 10.10.2.1 finds match");
    TEST_ASSERT_EQ(res2->next_hop, "eth2-datacenter", "LPM matches /16 datacenter prefix");

    // 3. Broad Class A Match (/8 match)
    auto res3 = table.lookup("10.200.1.1");
    TEST_ASSERT_TRUE(res3.has_value(), "Lookup 10.200.1.1 finds match");
    TEST_ASSERT_EQ(res3->next_hop, "eth1-broad", "LPM matches /8 class A prefix");

    // 4. Default Route Fallback (/0 match)
    auto res4 = table.lookup("8.8.8.8");
    TEST_ASSERT_TRUE(res4.has_value(), "Lookup 8.8.8.8 finds default route");
    TEST_ASSERT_EQ(res4->next_hop, "eth0-default", "Falls back to 0.0.0.0/0 default route");
}

// ── Test Suite 3: Token Bucket Rate Limiter ──────────────────────────────
void test_suite_rate_limiter() {
    std::cout << "--- Test Suite 3: Token Bucket Rate Limiter (RateLimiter.hpp) ---\n";

    TokenBucketRateLimiter limiter(5, 5); // 5 tokens capacity, 5 tokens/sec refill

    // Consume all 5 tokens
    for (int i = 0; i < 5; ++i) {
        TEST_ASSERT_TRUE(limiter.try_consume(), "Consume token within capacity");
    }

    // 6th request should be denied (rate-limited)
    TEST_ASSERT_FALSE(limiter.try_consume(), "6th request denied when bucket empty");

    // Sleep 400ms -> refills ~2 tokens
    std::this_thread::sleep_for(400ms);
    TEST_ASSERT_TRUE(limiter.try_consume(), "Consume refilled token after elapsed time");
}

// ── Test Suite 4: Session Authentication Subsystem ───────────────────────
void test_suite_auth_system() {
    std::cout << "--- Test Suite 4: Session Authentication (AuthSession.hpp) ---\n";

    const std::string secret = "qualcomm_test_secret";
    AuthSession session(10, secret);

    TEST_ASSERT_TRUE(session.state() == AuthState::CHALLENGE_SENT, "Initial state is CHALLENGE_SENT");
    TEST_ASSERT_TRUE(session.nonce_hex().length() == 16, "8-byte nonce hex length is 16 characters");

    // Submit correct token
    std::string valid_token = compute_token(session.nonce(), secret);
    TEST_ASSERT_TRUE(session.submit_token(valid_token), "Valid token accepted");
    TEST_ASSERT_TRUE(session.state() == AuthState::AUTHENTICATED, "State transitions to AUTHENTICATED");

    // Replay Attack Test against new session
    AuthSession new_session(11, secret);
    TEST_ASSERT_FALSE(new_session.submit_token(valid_token), "Replaying old token against new nonce fails");
    TEST_ASSERT_TRUE(new_session.state() == AuthState::REJECTED, "State transitions to REJECTED on bad token");
}

int main() {
    std::cout << "====================================================================================================\n";
    std::cout << " PHASE 20: AUTOMATED UNIT TESTING SUITE\n";
    std::cout << "====================================================================================================\n\n";

    test_suite_binary_protocol();
    test_suite_routing_engine();
    test_suite_rate_limiter();
    test_suite_auth_system();

    std::cout << "\n====================================================================================================\n";
    std::cout << " UNIT TEST SUMMARY: " << g_tests_passed << " Passed, " << g_tests_failed << " Failed.\n";
    std::cout << "====================================================================================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
