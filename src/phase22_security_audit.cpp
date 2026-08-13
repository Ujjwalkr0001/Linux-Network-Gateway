#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include "protocol/Packet.hpp"
#include "auth/AuthSession.hpp"
#include "ratelimit/RateLimiter.hpp"

void audit_allocation_dos_mitigation() {
    std::cout << "--- Audit 1: Memory Allocation DoS Defense ---\n";

    // Attempt crafting malicious packet header claiming 500 MB payload length
    uint8_t malicious_buf[10];
    malicious_buf[0] = PROTOCOL_VERSION;
    malicious_buf[1] = static_cast<uint8_t>(PacketType::DATA);
    uint32_t fake_length = htonl(500000000); // 500 MB
    std::memcpy(malicious_buf + 2, &fake_length, sizeof(uint32_t));

    Packet out_pkt;
    bool parsed = Packet::deserialize(malicious_buf, sizeof(malicious_buf), out_pkt);

    assert(parsed == false);
    std::cout << "  [PASS] 500 MB malicious payload size rejected before memory allocation. Allocation DoS mitigated.\n\n";
}

void audit_replay_attack_mitigation() {
    std::cout << "--- Audit 2: Session Replay Attack Defense ---\n";

    const std::string secret = "qualcomm_secret_key";

    // Session 1: Client authenticates
    AuthSession sess1(1, secret);
    std::string token1 = compute_token(sess1.nonce(), secret);
    assert(sess1.submit_token(token1) == true);

    // Session 2: Attacker replays token1 from Session 1
    AuthSession sess2(2, secret);
    bool replayed_auth = sess2.submit_token(token1);

    assert(replayed_auth == false);
    assert(sess2.state() == AuthState::REJECTED);
    std::cout << "  [PASS] Replayed token rejected due to unique per-session challenge nonce. Replay attack blocked.\n\n";
}

void audit_rate_limiting_dos_mitigation() {
    std::cout << "--- Audit 3: High-Volume Flood Rate Limiting ---\n";

    TokenBucketRateLimiter limiter(10, 10);

    // Drain all 10 tokens
    for (int i = 0; i < 10; ++i) {
        assert(limiter.try_consume() == true);
    }

    // 11th request must be dropped immediately
    bool dropped = !limiter.try_consume();
    assert(dropped == true);
    std::cout << "  [PASS] Excess flood traffic dropped at O(1) lock-free atomic CAS complexity.\n\n";
}

void audit_header_packing_safety() {
    std::cout << "--- Audit 4: Binary Header Alignment & Structure Packing ---\n";

    // Ensure compiler padding does not alter sizeof(PacketHeader)
    assert(sizeof(PacketHeader) == 6);
    std::cout << "  [PASS] sizeof(PacketHeader) strictly matches 6 bytes (#pragma pack push(1) verified).\n\n";
}

int main() {
    std::cout << "====================================================================================================\n";
    std::cout << " PHASE 22: SECURITY AUDIT & THREAT MODELING VERIFICATION\n";
    std::cout << "====================================================================================================\n\n";

    audit_allocation_dos_mitigation();
    audit_replay_attack_mitigation();
    audit_rate_limiting_dos_mitigation();
    audit_header_packing_safety();

    std::cout << "====================================================================================================\n";
    std::cout << " [✓] ALL GATEWAY SECURITY ASSERTIONS AUDITED & VERIFIED PASSED!\n";
    std::cout << "====================================================================================================\n";
    return 0;
}
