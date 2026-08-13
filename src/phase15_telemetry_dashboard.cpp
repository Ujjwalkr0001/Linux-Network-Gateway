#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdio>
#include "telemetry/Telemetry.hpp"

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────
// ANSI colour & cursor helpers
// ─────────────────────────────────────────────────────────────────────────
namespace term {
    constexpr const char* RESET   = "\033[0m";
    constexpr const char* BOLD    = "\033[1m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* RED     = "\033[31m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CLEAR   = "\033[2J\033[H"; // clear screen + move cursor home
    constexpr const char* HOME    = "\033[H";         // move cursor home only
}

// ─────────────────────────────────────────────────────────────────────────
// Render a horizontal bar gauge
//   value  : current value
//   max    : value that fills the bar
//   width  : character width of the bar
// ─────────────────────────────────────────────────────────────────────────
std::string bar_gauge(double value, double max_val, int width = 30) {
    int filled = (max_val > 0) ? static_cast<int>((value / max_val) * width) : 0;
    filled = std::min(filled, width);
    std::string bar(filled, '#');
    bar += std::string(width - filled, '-');
    return bar;
}

// ─────────────────────────────────────────────────────────────────────────
// Render the dashboard to stdout
// ─────────────────────────────────────────────────────────────────────────
void render_dashboard(const Telemetry::Snapshot& s) {
    // Move cursor home (redraw in place — no flicker)
    std::cout << term::HOME;

    // ── Header bar ────────────────────────────────────────────────────────
    std::cout << term::BOLD << term::CYAN
              << "╔══════════════════════════════════════════════════════╗\n"
              << "║      LINUX NETWORK GATEWAY  — LIVE TELEMETRY        ║\n"
              << "╚══════════════════════════════════════════════════════╝"
              << term::RESET << "\n\n";

    // ── Uptime & System Resources ───────────────────────────────────────
    int h = static_cast<int>(s.uptime_sec) / 3600;
    int m = (static_cast<int>(s.uptime_sec) % 3600) / 60;
    int sc = static_cast<int>(s.uptime_sec) % 60;
    std::cout << term::BOLD << "  Uptime             : " << term::RESET
              << std::setw(2) << std::setfill('0') << h << ":"
              << std::setw(2) << std::setfill('0') << m << ":"
              << std::setw(2) << std::setfill('0') << sc << "\n";

    // CPU Usage %
    const char* cpu_col = (s.cpu_usage_pct > 75.0) ? term::RED : term::GREEN;
    std::cout << term::BOLD << "  CPU Usage          : " << term::RESET
              << cpu_col << std::fixed << std::setprecision(1) << s.cpu_usage_pct << "%" << term::RESET << "\n";
    std::cout << "  [" << cpu_col << bar_gauge(s.cpu_usage_pct, 100.0, 40) << term::RESET << "]\n";

    // RSS Memory MB
    std::cout << term::BOLD << "  RSS Memory Footprint: " << term::RESET
              << term::MAGENTA << std::fixed << std::setprecision(2) << s.rss_memory_mb << " MB" << term::RESET << "\n";
    std::cout << "  [" << term::MAGENTA << bar_gauge(s.rss_memory_mb, 250.0, 40) << term::RESET << "]\n\n";

    // ── Connections & Latency ─────────────────────────────────────────────
    std::cout << term::BOLD << "  Active Connections : " << term::RESET
              << term::GREEN << s.active_connections << term::RESET << "\n";

    std::cout << term::BOLD << "  Avg Processing Latency: " << term::RESET
              << term::CYAN << std::fixed << std::setprecision(2) << s.avg_latency_us << " us" << term::RESET << "\n";

    // ── Throughput ───────────────────────────────────────────────────────
    std::cout << term::BOLD << "  Throughput         : " << term::RESET
              << term::GREEN << std::fixed << std::setprecision(1)
              << s.pkt_per_sec_in << " pkt/s" << term::RESET << "\n";

    // PPS bar gauge (scale: 0–10000 pkt/s)
    std::cout << "  [" << term::GREEN
              << bar_gauge(s.pkt_per_sec_in, 10000.0, 40)
              << term::RESET << "] " << s.pkt_per_sec_in << " / 10k\n\n";

    // ── Packet counters ──────────────────────────────────────────────────
    std::cout << term::BOLD << "  Packets RX         : " << term::RESET
              << s.packets_received << "\n";
    std::cout << term::BOLD << "  Packets TX         : " << term::RESET
              << s.packets_sent << "\n";

    // ── Drop rate ────────────────────────────────────────────────────────
    const char* drop_col = (s.drop_rate_pct > 5.0) ? term::RED : term::GREEN;
    std::cout << term::BOLD << "  Packets Dropped    : " << term::RESET
              << drop_col << s.packets_dropped
              << "  (" << std::fixed << std::setprecision(2)
              << s.drop_rate_pct << "%)" << term::RESET << "\n";

    // Drop bar gauge (scale: 0–100%)
    const char* bar_col = (s.drop_rate_pct > 5.0) ? term::RED : term::YELLOW;
    std::cout << "  [" << bar_col
              << bar_gauge(s.drop_rate_pct, 100.0, 40)
              << term::RESET << "] " << s.drop_rate_pct << "% drop\n\n";

    // ── Byte counters ────────────────────────────────────────────────────
    auto fmt_bytes = [](uint64_t b) -> std::string {
        std::ostringstream oss;
        if (b >= 1024*1024*1024) oss << std::fixed << std::setprecision(2) << b/(1024.0*1024*1024) << " GB";
        else if (b >= 1024*1024) oss << std::fixed << std::setprecision(2) << b/(1024.0*1024) << " MB";
        else if (b >= 1024)      oss << std::fixed << std::setprecision(2) << b/1024.0 << " KB";
        else                     oss << b << " B";
        return oss.str();
    };

    std::cout << term::BOLD << "  Bytes IN           : " << term::RESET
              << term::CYAN << fmt_bytes(s.bytes_in) << term::RESET << "\n";
    std::cout << term::BOLD << "  Bytes OUT          : " << term::RESET
              << term::CYAN << fmt_bytes(s.bytes_out) << term::RESET << "\n\n";

    // ── Security ─────────────────────────────────────────────────────────
    const char* auth_col = (s.auth_failures > 0) ? term::RED : term::GREEN;
    std::cout << term::BOLD << "  Auth Failures      : " << term::RESET
              << auth_col << s.auth_failures << term::RESET << "\n\n";

    std::cout << term::YELLOW
              << "  [Refreshing every 1s — Ctrl+C to quit]\n"
              << term::RESET;
    std::cout.flush();
}

// ─────────────────────────────────────────────────────────────────────────
// Simulate gateway traffic in background threads
// ─────────────────────────────────────────────────────────────────────────
void simulate_traffic(Telemetry& tel, std::atomic<bool>& running) {
    // Simulate 3 client connections opened
    tel.set_active_connections(3);

    uint64_t pkt = 0;
    while (running) {
        // Simulate incoming packets (burst then steady)
        for (int i = 0; i < 50; ++i) {
            tel.record_packet_in(512 + (pkt % 256));   // 512–768 byte payloads
            tel.record_packet_out(256);
            tel.record_latency(12 + (pkt % 15));       // 12-27 microsecond latency
            ++pkt;
        }
        // Simulate occasional drops (every ~500 pkts)
        if (pkt % 500 < 5) tel.record_drop();

        // Simulate auth failure every 2000 packets
        if (pkt % 2000 == 0 && pkt > 0) tel.record_auth_failure();

        std::this_thread::sleep_for(10ms);
    }
}

int main() {
    // Clear screen once at startup
    std::cout << term::CLEAR;
    std::cout << term::BOLD << term::CYAN
              << "  Starting Gateway Telemetry Dashboard...\n"
              << term::RESET;
    std::this_thread::sleep_for(500ms);

    Telemetry& tel = get_telemetry();
    std::atomic<bool> running{true};

    // Background traffic simulator
    std::thread traffic_thread([&]() { simulate_traffic(tel, running); });

    // Dashboard render loop — refresh every 1 second, run for 10 iterations
    for (int tick = 0; tick < 10; ++tick) {
        std::this_thread::sleep_for(1s);
        auto snap = tel.take_snapshot();
        render_dashboard(snap);
    }

    running = false;
    traffic_thread.join();

    std::cout << "\n\n[+] Phase 15 complete. Telemetry dashboard demonstrated.\n";
    return 0;
}
