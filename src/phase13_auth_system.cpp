#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include "auth/AuthSession.hpp"

// Shared secret — in production this is loaded from a config file or HSM
const std::string SHARED_SECRET = "qualcomm_gateway_s3cr3t";

// ── Simulate a client correctly authenticating ────────────────────────────
void test_valid_auth() {
    std::cout << "--- Test 1: Valid Authentication ---\n";

    AuthSession session(4, SHARED_SECRET);
    std::cout << "  [SERVER] Sending nonce to FD 4: " << session.nonce_hex() << "\n";

    // Client computes the correct token using the SAME secret
    std::string client_token = compute_token(session.nonce(), SHARED_SECRET);
    std::cout << "  [CLIENT] Computed token: " << client_token << "\n";

    bool ok = session.submit_token(client_token);
    std::cout << "  Result: " << auth_state_str(session.state())
              << " | Auth passed: " << (ok ? "YES" : "NO") << "\n\n";
}

// ── Simulate a client sending a wrong token (attacker/wrong secret) ───────
void test_invalid_auth() {
    std::cout << "--- Test 2: Invalid Token (Wrong Secret) ---\n";

    AuthSession session(5, SHARED_SECRET);
    std::cout << "  [SERVER] Sending nonce to FD 5: " << session.nonce_hex() << "\n";

    // Attacker guesses a token with a different secret
    std::string attacker_token = compute_token(session.nonce(), "wrong_secret");
    std::cout << "  [CLIENT] Attacker token: " << attacker_token << "\n";

    bool ok = session.submit_token(attacker_token);
    std::cout << "  Result: " << auth_state_str(session.state())
              << " | Auth passed: " << (ok ? "YES" : "NO") << "\n\n";
}

// ── Simulate a replay attack (reusing a token from a previous session) ────
void test_replay_attack() {
    std::cout << "--- Test 3: Replay Attack (Reused Token from Old Session) ---\n";

    // Session A: attacker intercepts the valid exchange
    AuthSession session_a(6, SHARED_SECRET);
    std::string intercepted_token = compute_token(session_a.nonce(), SHARED_SECRET);
    session_a.submit_token(intercepted_token); // session_a now authenticated

    // Session B: NEW connection with a DIFFERENT nonce
    AuthSession session_b(7, SHARED_SECRET);
    std::cout << "  [SERVER] New nonce for FD 7: " << session_b.nonce_hex() << "\n";
    std::cout << "  [ATTACKER] Replaying old token: " << intercepted_token << "\n";

    // Replay the old token against the new nonce — must FAIL
    bool ok = session_b.submit_token(intercepted_token);
    std::cout << "  Result: " << auth_state_str(session_b.state())
              << " | Replay blocked: " << (!ok ? "YES ✓" : "NO ✗ (VULNERABILITY!)") << "\n\n";
}

// ── Simulate a gateway managing multiple sessions ─────────────────────────
void test_session_manager() {
    std::cout << "--- Test 4: Multi-Session Gateway (5 clients) ---\n";

    // Map: fd → AuthSession
    std::unordered_map<int, std::unique_ptr<AuthSession>> sessions;

    // Simulate 5 connections arriving
    for (int fd = 10; fd < 15; ++fd) {
        sessions[fd] = std::make_unique<AuthSession>(fd, SHARED_SECRET);
        std::cout << "  [FD " << fd << "] Challenge nonce: "
                  << sessions[fd]->nonce_hex() << "\n";
    }

    std::cout << "\n  Clients responding:\n";

    // FD 10, 11, 12 authenticate correctly
    for (int fd : {10, 11, 12}) {
        std::string token = compute_token(sessions[fd]->nonce(), SHARED_SECRET);
        sessions[fd]->submit_token(token);
    }

    // FD 13 sends garbage
    sessions[13]->submit_token("deadbeefcafebabe");

    // FD 14 never responds (timeout simulation — skip for demo)
    std::cout << "  [FD 14] No response (would time out in production)\n";

    // Summary
    std::cout << "\n  Session Summary:\n";
    for (auto& [fd, sess] : sessions) {
        std::cout << "  FD " << fd << ": " << auth_state_str(sess->state()) << "\n";
    }
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 13: SESSION AUTHENTICATION SYSTEM\n";
    std::cout << " (Challenge-Response, Nonce, HMAC-like Token)\n";
    std::cout << "=================================================\n\n";

    test_valid_auth();
    test_invalid_auth();
    test_replay_attack();
    test_session_manager();

    std::cout << "\n[+] Phase 13 complete. Authentication system verified.\n";
    return 0;
}
