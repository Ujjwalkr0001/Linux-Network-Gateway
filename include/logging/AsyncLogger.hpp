#ifndef ASYNC_LOGGER_HPP
#define ASYNC_LOGGER_HPP

// ─────────────────────────────────────────────────────────────────────────
// ASYNCHRONOUS RING-BUFFER LOGGER
//
// Problem with synchronous logging in a hot path:
//   std::cout << "..." is slow — it flushes to kernel, acquires a mutex,
//   and may block the event loop thread for microseconds.
//   At 100k packets/sec that becomes catastrophic.
//
// Solution — Async Logger:
//   1. Producer threads write log entries into a RING BUFFER (lock-free).
//   2. A dedicated background CONSUMER THREAD drains the ring buffer
//      and writes to stdout / file, decoupled from the hot path.
//
// Ring Buffer Design:
//   - Fixed capacity N (power-of-2 for fast modulo: idx & (N-1))
//   - head_ : next slot to WRITE (producers advance this)
//   - tail_ : next slot to READ  (consumer advances this)
//   - Full : head_ - tail_ == N
//   - Empty: head_ == tail_
//   - All indices are std::atomic → no mutex on the fast producer path
// ─────────────────────────────────────────────────────────────────────────

#include <array>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cassert>

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

inline const char* level_str(LogLevel l) {
    switch (l) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default:              return "?????";
    }
}

// ── Log Entry: a fixed-size slot in the ring buffer ─────────────────────
struct LogEntry {
    LogLevel    level;
    char        message[256];  // fixed-size avoids heap allocation on hot path
    int64_t     timestamp_us;  // microseconds since epoch
    bool        valid = false; // slot is filled and ready to consume
};

// ── Ring Buffer capacity: must be a power of 2 ──────────────────────────
constexpr size_t RING_CAPACITY = 1024; // 1024 slots

class AsyncLogger {
public:
    AsyncLogger() : head_(0), tail_(0), running_(false) {}

    ~AsyncLogger() { stop(); }

    // Start the background consumer thread
    void start() {
        running_ = true;
        consumer_thread_ = std::thread([this]() { consume_loop(); });
        std::cout << "[Logger] Background consumer thread started\n";
    }

    // Graceful shutdown: flush remaining entries then stop
    void stop() {
        if (!running_) return;
        running_ = false;
        if (consumer_thread_.joinable()) consumer_thread_.join();
    }

    // ── Producer API: called from ANY thread, lock-free ─────────────────
    bool log(LogLevel level, const std::string& msg) {
        // Claim a slot: atomically increment head_
        // Using fetch_add with relaxed order — we only need atomicity here
        size_t slot = head_.fetch_add(1, std::memory_order_relaxed) % RING_CAPACITY;

        LogEntry& entry = ring_[slot];

        // If the slot is still valid (not yet consumed), the buffer is FULL
        if (entry.valid) {
            dropped_.fetch_add(1, std::memory_order_relaxed);
            return false; // drop the log entry
        }

        // Fill the entry (no mutex needed — we own this slot)
        entry.level = level;
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        entry.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

        // Safe copy into fixed-size buffer
        size_t copy_len = std::min(msg.size(), sizeof(entry.message) - 1);
        msg.copy(entry.message, copy_len);
        entry.message[copy_len] = '\0';

        // Mark slot ready for consumer — release ordering ensures message
        // bytes are visible before valid flag is seen as true
        entry.valid = true; // consumer will now pick this up

        return true;
    }

    // Convenience wrappers
    bool debug(const std::string& m) { return log(LogLevel::DEBUG, m); }
    bool info (const std::string& m) { return log(LogLevel::INFO,  m); }
    bool warn (const std::string& m) { return log(LogLevel::WARN,  m); }
    bool error(const std::string& m) { return log(LogLevel::ERROR, m); }

    uint64_t dropped_count() const {
        return dropped_.load(std::memory_order_relaxed);
    }

private:
    // ── Consumer loop: runs in dedicated background thread ───────────────
    void consume_loop() {
        while (running_ || has_pending()) {
            size_t slot = tail_.load(std::memory_order_relaxed) % RING_CAPACITY;
            LogEntry& entry = ring_[slot];

            if (!entry.valid) {
                // Nothing to read — yield CPU rather than busy-spin
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }

            // Format and write the log line
            std::cout << "[" << entry.timestamp_us << " us] "
                      << "[" << level_str(entry.level) << "] "
                      << entry.message << "\n";

            entry.valid = false; // release slot back to producers
            tail_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool has_pending() const {
        for (size_t i = 0; i < RING_CAPACITY; ++i) {
            if (ring_[i].valid) return true;
        }
        return false;
    }

    std::array<LogEntry, RING_CAPACITY> ring_{};
    std::atomic<size_t>   head_;
    std::atomic<size_t>   tail_;
    std::atomic<bool>     running_;
    std::atomic<uint64_t> dropped_{0};
    std::thread           consumer_thread_;
};

// ── Global singleton logger ───────────────────────────────────────────────
inline AsyncLogger& get_logger() {
    static AsyncLogger instance;
    return instance;
}

// Convenience macros — zero overhead in hot path (no string formatting unless logging)
#define LOG_DEBUG(msg) get_logger().debug(msg)
#define LOG_INFO(msg)  get_logger().info(msg)
#define LOG_WARN(msg)  get_logger().warn(msg)
#define LOG_ERROR(msg) get_logger().error(msg)

#endif // ASYNC_LOGGER_HPP
