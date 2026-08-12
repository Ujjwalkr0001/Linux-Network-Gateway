#ifndef RATE_LIMITER_HPP
#define RATE_LIMITER_HPP

// ─────────────────────────────────────────────────────────────────
// TOKEN BUCKET RATE LIMITER
//
// Algorithm:
//   - A "bucket" holds up to `capacity` tokens.
//   - Tokens are added at `refill_rate` tokens per second.
//   - Each packet/request consumes 1 token.
//   - If the bucket is empty → request is DENIED (rate-limited).
//
// Why std::atomic?
//   The gateway runs multiple threads (reactor + worker threads).
//   Without atomics, two threads reading/writing `tokens_` would
//   cause a data race (undefined behavior). std::atomic<int64_t>
//   guarantees each read-modify-write is indivisible at the hardware
//   level, without needing a mutex (faster, lock-free).
//
// Why int64_t and not double?
//   Floating point operations are NOT atomic on all architectures.
//   We store tokens as integer micro-tokens (×1,000,000) to keep
//   sub-token precision while staying lock-free.
// ─────────────────────────────────────────────────────────────────

#include <atomic>
#include <chrono>
#include <string>
#include <iostream>
#include <cstdint>

class TokenBucketRateLimiter {
public:
    // capacity_tokens   : maximum burst size (e.g. 100 packets)
    // refill_rate_per_s : steady-state rate  (e.g. 50 packets/sec)
    TokenBucketRateLimiter(int64_t capacity_tokens, int64_t refill_rate_per_s)
        : capacity_micro_(capacity_tokens * MICRO),
          refill_rate_per_s_micro_(refill_rate_per_s * MICRO),
          tokens_micro_(capacity_tokens * MICRO)       // start full
    {
        last_refill_ = std::chrono::steady_clock::now();
    }

    // Returns true if the request is allowed, false if rate-limited.
    // Thread-safe: uses atomic CAS loop + single mutex-free refill.
    bool try_consume(int64_t tokens_needed = 1) {
        refill(); // bring bucket up to date with elapsed time

        // CAS (Compare-And-Swap) loop:
        //   Read current value → compute desired → swap atomically.
        //   If another thread changed the value between our read and
        //   swap, the CAS fails and we retry.
        int64_t current = tokens_micro_.load(std::memory_order_relaxed);
        while (true) {
            int64_t needed_micro = tokens_needed * MICRO;
            if (current < needed_micro) {
                return false; // bucket empty → rate-limited
            }
            int64_t desired = current - needed_micro;
            if (tokens_micro_.compare_exchange_weak(
                    current, desired,
                    std::memory_order_release,
                    std::memory_order_relaxed)) {
                return true; // successfully consumed tokens
            }
            // CAS failed → current was updated with the actual value, retry
        }
    }

    // Current token level (for diagnostics)
    double current_tokens() const {
        return static_cast<double>(tokens_micro_.load(std::memory_order_relaxed)) / MICRO;
    }

    // Refill tokens based on elapsed time since last refill
    void refill() {
        auto now     = std::chrono::steady_clock::now();
        // Compute elapsed microseconds since last refill
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                           now - last_refill_.load()).count();

        if (elapsed <= 0) return;

        // Tokens to add = rate × elapsed_seconds
        // = (rate_micro / 1,000,000) × (elapsed_us / 1,000,000) × MICRO
        // Simplified: rate_micro × elapsed_us / 1,000,000
        int64_t to_add = (refill_rate_per_s_micro_ * elapsed) / 1'000'000LL;
        if (to_add <= 0) return;

        // Update last_refill_ timestamp (best-effort, no strict atomicity needed here)
        last_refill_.store(now);

        // Add tokens, capping at capacity
        int64_t current = tokens_micro_.load(std::memory_order_relaxed);
        while (true) {
            int64_t desired = std::min(current + to_add, capacity_micro_);
            if (tokens_micro_.compare_exchange_weak(
                    current, desired,
                    std::memory_order_release,
                    std::memory_order_relaxed)) {
                break;
            }
        }
    }

private:
    static constexpr int64_t MICRO = 1'000'000LL; // micro-token scale factor

    const int64_t capacity_micro_;
    const int64_t refill_rate_per_s_micro_;

    std::atomic<int64_t> tokens_micro_;
    std::atomic<std::chrono::steady_clock::time_point> last_refill_;
};

#endif // RATE_LIMITER_HPP
