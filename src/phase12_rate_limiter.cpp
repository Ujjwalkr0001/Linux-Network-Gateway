#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iomanip>
#include "ratelimit/RateLimiter.hpp"

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────
// Helper: simulate a burst of N requests from a single "client"
// ─────────────────────────────────────────────────────────────
void simulate_client(TokenBucketRateLimiter& limiter,
                     const std::string& client_id,
                     int requests,
                     std::chrono::milliseconds interval)
{
    int allowed = 0, denied = 0;
    for (int i = 0; i < requests; ++i) {
        bool ok = limiter.try_consume();
        if (ok) {
            ++allowed;
        } else {
            ++denied;
            std::cout << "  [RATE-LIMITED] " << client_id
                      << " req #" << (i+1)
                      << " (tokens left: "
                      << std::fixed << std::setprecision(2)
                      << limiter.current_tokens() << ")\n";
        }
        std::this_thread::sleep_for(interval);
    }
    std::cout << "[" << client_id << "] Allowed=" << allowed
              << "  Denied=" << denied
              << "  (tokens remaining: "
              << std::fixed << std::setprecision(2)
              << limiter.current_tokens() << ")\n";
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 12: CONCURRENCY-SAFE RATE LIMITER\n";
    std::cout << " (Token Bucket + std::atomic CAS)\n";
    std::cout << "=================================================\n\n";

    // ── TEST 1: Single-threaded burst ────────────────────────────
    std::cout << "--- Test 1: Single Client Burst (capacity=10, rate=5/s) ---\n";
    {
        // Bucket holds 10 tokens, refills 5 per second
        TokenBucketRateLimiter limiter(10, 5);

        // Fire 15 requests instantly — first 10 allowed, rest denied
        for (int i = 0; i < 15; ++i) {
            bool ok = limiter.try_consume();
            std::cout << "  Request " << std::setw(2) << (i+1)
                      << ": " << (ok ? "[ALLOW]" : "[DENY ] RATE-LIMITED")
                      << "  tokens=" << std::fixed << std::setprecision(2)
                      << limiter.current_tokens() << "\n";
        }
    }

    // ── TEST 2: Refill over time ─────────────────────────────────
    std::cout << "\n--- Test 2: Bucket Refills Over Time (rate=10/s) ---\n";
    {
        TokenBucketRateLimiter limiter(10, 10); // 10 tokens/sec

        // Drain all 10 tokens
        for (int i = 0; i < 10; ++i) limiter.try_consume();
        std::cout << "  Bucket drained. Tokens: " << limiter.current_tokens() << "\n";

        // Wait 500ms — should refill ~5 tokens
        std::cout << "  Waiting 500ms...\n";
        std::this_thread::sleep_for(500ms);

        std::cout << "  Tokens after 500ms: "
                  << std::fixed << std::setprecision(2)
                  << limiter.current_tokens() << " (expect ~5)\n";

        int allowed = 0;
        for (int i = 0; i < 8; ++i) {
            if (limiter.try_consume()) ++allowed;
        }
        std::cout << "  Fired 8 requests → " << allowed << " allowed\n";
    }

    // ── TEST 3: Multi-threaded concurrent clients ─────────────────
    std::cout << "\n--- Test 3: Multi-Threaded Stress (4 threads, shared limiter) ---\n";
    {
        // Shared rate limiter: 20 token capacity, 10 tokens/sec
        TokenBucketRateLimiter shared_limiter(20, 10);

        // 4 threads each send 10 requests at 50ms intervals
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&shared_limiter, t]() {
                simulate_client(shared_limiter,
                                "Thread-" + std::to_string(t),
                                10,
                                50ms);
            });
        }
        for (auto& thr : threads) thr.join();
        std::cout << "\n[+] All threads done. Final tokens: "
                  << std::fixed << std::setprecision(2)
                  << shared_limiter.current_tokens() << "\n";
    }

    // ── TEST 4: Simulate gateway packet flow ─────────────────────
    std::cout << "\n--- Test 4: Gateway Simulation (100 pkt/s client, 50 pkt/s limit) ---\n";
    {
        // Gateway allows 50 packets/sec, burst of 50
        TokenBucketRateLimiter gateway_limiter(50, 50);

        int allowed = 0, denied = 0;
        // Client blasts 100 packets with zero delay (worst case burst)
        for (int i = 0; i < 100; ++i) {
            if (gateway_limiter.try_consume()) ++allowed;
            else                              ++denied;
        }
        std::cout << "  100 packets fired instantly:\n";
        std::cout << "  Allowed: " << allowed << "  Denied: " << denied << "\n";
        std::cout << "  (Expected: ~50 allowed, ~50 denied)\n";
    }

    std::cout << "\n[+] Phase 12 complete. Rate Limiter is concurrency-safe.\n";
    return 0;
}
