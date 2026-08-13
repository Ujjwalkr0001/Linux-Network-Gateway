#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

// ─────────────────────────────────────────────────────────────────────────
// TELEMETRY SUBSYSTEM
//
// Tracks real-time gateway statistics with lock-free std::atomic counters.
// Any thread (reactor, worker, rate-limiter) can safely increment counters
// without a mutex — no contention, no blocking.
//
// Metrics captured:
//   - packets_received  : total inbound packets processed
//   - packets_dropped   : dropped by rate-limiter or buffer overflow
//   - bytes_in / out    : total bytes transferred
//   - active_connections: current live TCP connections
//   - auth_failures     : rejected authentication attempts
//   - uptime            : calculated from start_time_
//   - cpu_usage_pct     : process CPU utilization dynamically from /proc/self/stat
//   - rss_memory_mb     : Resident Set Size memory footprint from /proc/self/status
//   - avg_latency_us    : packet processing latency in microseconds
// ─────────────────────────────────────────────────────────────────────────

#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <algorithm>

#if defined(__linux__) || defined(__gnu_linux__) || defined(__WSL__)
#include <unistd.h>
#endif

class Telemetry {
public:
    Telemetry() {
        start_time_ = std::chrono::steady_clock::now();
        last_cpu_time_ = std::chrono::steady_clock::now();
        last_utime_stime_ = 0;
        reset();
    }

    // ── Counters (called from hot path — must be lock-free) ─────────────
    void record_packet_in (uint64_t bytes = 0) {
        packets_received_.fetch_add(1, std::memory_order_relaxed);
        bytes_in_.fetch_add(bytes, std::memory_order_relaxed);
    }
    void record_packet_out(uint64_t bytes = 0) {
        packets_sent_.fetch_add(1, std::memory_order_relaxed);
        bytes_out_.fetch_add(bytes, std::memory_order_relaxed);
    }
    void record_drop() {
        packets_dropped_.fetch_add(1, std::memory_order_relaxed);
    }
    void record_auth_failure() {
        auth_failures_.fetch_add(1, std::memory_order_relaxed);
    }
    void record_latency(uint64_t micros) {
        total_latency_us_.fetch_add(micros, std::memory_order_relaxed);
        latency_samples_.fetch_add(1, std::memory_order_relaxed);
    }
    void set_active_connections(int64_t n) {
        active_connections_.store(n, std::memory_order_relaxed);
    }
    void connection_opened() {
        active_connections_.fetch_add(1, std::memory_order_relaxed);
    }
    void connection_closed() {
        active_connections_.fetch_sub(1, std::memory_order_relaxed);
    }

    // ── System Metrics (/proc filesystem parsing) ───────────────────────

    // Read Resident Set Size (RSS) memory in MB from /proc/self/status
    double get_rss_memory_mb() {
        std::ifstream status_file("/proc/self/status");
        if (!status_file.is_open()) return 0.0;

        std::string line;
        while (std::getline(status_file, line)) {
            if (line.rfind("VmRSS:", 0) == 0) {
                std::istringstream iss(line);
                std::string key;
                uint64_t kb = 0;
                iss >> key >> kb;
                return static_cast<double>(kb) / 1024.0;
            }
        }
        return 0.0;
    }

    // Calculate Process CPU Utilization % from /proc/self/stat
    double get_cpu_usage_pct() {
        std::ifstream stat_file("/proc/self/stat");
        if (!stat_file.is_open()) return 0.0;

        std::string dummy;
        uint64_t utime = 0, stime = 0;
        // Skip first 13 fields to reach utime (14th) and stime (15th)
        for (int i = 1; i <= 13; ++i) {
            stat_file >> dummy;
        }
        stat_file >> utime >> stime;

        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - last_cpu_time_).count();
        if (elapsed_sec <= 0.0) return 0.0;

        uint64_t current_total = utime + stime;
        uint64_t delta_jiffies = current_total - last_utime_stime_;

        last_cpu_time_ = now;
        last_utime_stime_ = current_total;

        long ticks_per_sec = 100;
#if defined(_SC_CLK_TCK)
        ticks_per_sec = sysconf(_SC_CLK_TCK);
        if (ticks_per_sec <= 0) ticks_per_sec = 100;
#endif

        double process_cpu_sec = static_cast<double>(delta_jiffies) / ticks_per_sec;
        double cpu_pct = (process_cpu_sec / elapsed_sec) * 100.0;
        return std::max(0.0, cpu_pct);
    }

    // ── Snapshot: read all counters at once ─────────────────────────────
    struct Snapshot {
        uint64_t packets_received;
        uint64_t packets_sent;
        uint64_t packets_dropped;
        uint64_t bytes_in;
        uint64_t bytes_out;
        int64_t  active_connections;
        uint64_t auth_failures;
        double   uptime_sec;
        double   pkt_per_sec_in;   // computed from delta vs last snapshot
        double   drop_rate_pct;    // dropped / (received + dropped) * 100
        double   cpu_usage_pct;    // CPU usage percentage
        double   rss_memory_mb;    // RSS memory footprint in MB
        double   avg_latency_us;   // Average packet latency in microseconds
    };

    Snapshot take_snapshot() {
        auto now = std::chrono::steady_clock::now();
        double uptime = std::chrono::duration<double>(now - start_time_).count();

        uint64_t rx  = packets_received_.load(std::memory_order_relaxed);
        uint64_t tx  = packets_sent_.load(std::memory_order_relaxed);
        uint64_t drp = packets_dropped_.load(std::memory_order_relaxed);
        uint64_t bin = bytes_in_.load(std::memory_order_relaxed);
        uint64_t bou = bytes_out_.load(std::memory_order_relaxed);
        int64_t  ac  = active_connections_.load(std::memory_order_relaxed);
        uint64_t af  = auth_failures_.load(std::memory_order_relaxed);
        uint64_t lat_us  = total_latency_us_.load(std::memory_order_relaxed);
        uint64_t samples = latency_samples_.load(std::memory_order_relaxed);

        double pps     = uptime > 0.0 ? static_cast<double>(rx) / uptime : 0.0;
        double total   = static_cast<double>(rx + drp);
        double drop_r  = total > 0.0 ? (static_cast<double>(drp) / total * 100.0) : 0.0;
        double cpu_pct = get_cpu_usage_pct();
        double rss_mb  = get_rss_memory_mb();
        double avg_lat = samples > 0 ? (static_cast<double>(lat_us) / samples) : 0.0;

        return Snapshot{ rx, tx, drp, bin, bou, ac, af, uptime, pps, drop_r, cpu_pct, rss_mb, avg_lat };
    }

    void reset() {
        packets_received_.store(0, std::memory_order_relaxed);
        packets_sent_.store(0, std::memory_order_relaxed);
        packets_dropped_.store(0, std::memory_order_relaxed);
        bytes_in_.store(0, std::memory_order_relaxed);
        bytes_out_.store(0, std::memory_order_relaxed);
        active_connections_.store(0, std::memory_order_relaxed);
        auth_failures_.store(0, std::memory_order_relaxed);
        total_latency_us_.store(0, std::memory_order_relaxed);
        latency_samples_.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> packets_received_{0};
    std::atomic<uint64_t> packets_sent_{0};
    std::atomic<uint64_t> packets_dropped_{0};
    std::atomic<uint64_t> bytes_in_{0};
    std::atomic<uint64_t> bytes_out_{0};
    std::atomic<int64_t>  active_connections_{0};
    std::atomic<uint64_t> auth_failures_{0};
    std::atomic<uint64_t> total_latency_us_{0};
    std::atomic<uint64_t> latency_samples_{0};
    std::chrono::steady_clock::time_point start_time_;

    std::chrono::steady_clock::time_point last_cpu_time_;
    uint64_t last_utime_stime_{0};
};

// ── Global singleton ─────────────────────────────────────────────────────
inline Telemetry& get_telemetry() {
    static Telemetry instance;
    return instance;
}

#endif // TELEMETRY_HPP
