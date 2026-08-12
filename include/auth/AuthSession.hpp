#ifndef AUTH_SESSION_HPP
#define AUTH_SESSION_HPP

// ─────────────────────────────────────────────────────────────────────────
// SESSION AUTHENTICATION SYSTEM
//
// Architecture:
//   Each new TCP connection starts in CHALLENGE_SENT state.
//   The server sends a random challenge nonce (8 bytes).
//   The client must reply with HMAC-SHA256(nonce, shared_secret).
//   On match  → session transitions to AUTHENTICATED.
//   On failure → session transitions to REJECTED (socket closed).
//
// Why HMAC and not plain password?
//   Plain password: replayed by a network attacker → auth bypass.
//   HMAC(nonce, secret): nonce is unique per session → replay-proof.
//   Even if attacker sniffs the response, they cannot reuse it.
//
// Note: We implement a simplified HMAC-like XOR-fold hash for teaching.
//       In production you would use OpenSSL's HMAC-SHA256.
// ─────────────────────────────────────────────────────────────────────────

#include <string>
#include <cstdint>
#include <array>
#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

// ── Nonce: 8 random bytes, unique per session ────────────────────────────
using Nonce = std::array<uint8_t, 8>;

inline Nonce generate_nonce() {
    // Use a hardware-seeded PRNG for the nonce
    std::random_device rd;
    std::mt19937_64 rng(rd());
    uint64_t raw = rng();
    Nonce n;
    for (int i = 0; i < 8; ++i) {
        n[i] = static_cast<uint8_t>((raw >> (i * 8)) & 0xFF);
    }
    return n;
}

inline std::string nonce_to_hex(const Nonce& n) {
    std::ostringstream oss;
    for (auto b : n) oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}

// ── Simplified HMAC: XOR-fold of nonce bytes with secret bytes ──────────
// Real production: use HMAC-SHA256 via OpenSSL/libsodium.
// Teaching version: deterministic, demonstratable without dependencies.
inline std::string compute_token(const Nonce& nonce, const std::string& secret) {
    std::array<uint8_t, 8> result{};
    for (int i = 0; i < 8; ++i) {
        result[i] = nonce[i] ^ static_cast<uint8_t>(secret[i % secret.size()]);
    }
    // Also fold in a second pass (mimics HMAC's inner/outer hash rounds)
    for (int i = 0; i < 8; ++i) {
        result[i] ^= static_cast<uint8_t>(secret[(i + 3) % secret.size()]);
    }
    std::ostringstream oss;
    for (auto b : result) oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}

// ── Auth Session States ──────────────────────────────────────────────────
enum class AuthState {
    CHALLENGE_SENT,   // Server sent nonce, awaiting client response
    AUTHENTICATED,    // Client proved knowledge of shared secret
    REJECTED,         // Wrong token → connection must be closed
    TIMED_OUT         // Client took too long to respond
};

inline std::string auth_state_str(AuthState s) {
    switch (s) {
        case AuthState::CHALLENGE_SENT:  return "CHALLENGE_SENT";
        case AuthState::AUTHENTICATED:   return "AUTHENTICATED";
        case AuthState::REJECTED:        return "REJECTED";
        case AuthState::TIMED_OUT:       return "TIMED_OUT";
        default:                         return "UNKNOWN";
    }
}

// ── AuthSession: per-connection auth context ─────────────────────────────
class AuthSession {
public:
    explicit AuthSession(int fd, const std::string& shared_secret)
        : fd_(fd), secret_(shared_secret), state_(AuthState::CHALLENGE_SENT) {
        nonce_ = generate_nonce();
        created_at_ = std::chrono::steady_clock::now();
        expected_token_ = compute_token(nonce_, secret_);
    }

    int fd() const { return fd_; }
    AuthState state() const { return state_; }
    const Nonce& nonce() const { return nonce_; }
    std::string nonce_hex() const { return nonce_to_hex(nonce_); }
    const std::string& expected_token() const { return expected_token_; }

    // Validate the token submitted by the client
    bool submit_token(const std::string& client_token) {
        if (state_ != AuthState::CHALLENGE_SENT) return false;

        if (is_timed_out()) {
            state_ = AuthState::TIMED_OUT;
            std::cout << "  [AUTH] FD " << fd_ << ": TIMED OUT\n";
            return false;
        }

        if (client_token == expected_token_) {
            state_ = AuthState::AUTHENTICATED;
            std::cout << "  [AUTH] FD " << fd_ << ": AUTHENTICATED ✓\n";
            return true;
        } else {
            state_ = AuthState::REJECTED;
            std::cout << "  [AUTH] FD " << fd_ << ": REJECTED ✗"
                      << " (got=" << client_token
                      << " expected=" << expected_token_ << ")\n";
            return false;
        }
    }

    bool is_timed_out(std::chrono::seconds timeout = std::chrono::seconds(5)) const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - created_at_) >= timeout;
    }

private:
    int fd_;
    std::string secret_;
    Nonce nonce_;
    std::string expected_token_;
    AuthState state_;
    std::chrono::steady_clock::time_point created_at_;
};

#endif // AUTH_SESSION_HPP
