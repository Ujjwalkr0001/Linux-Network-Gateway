#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <chrono>
#include "logging/AsyncLogger.hpp"

using namespace std::chrono_literals;

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 14: ASYNCHRONOUS RING-BUFFER LOGGER\n";
    std::cout << "=================================================\n\n";

    AsyncLogger& logger = get_logger();
    logger.start();

    // Brief pause so consumer thread is ready
    std::this_thread::sleep_for(10ms);

    // ── Test 1: Basic log levels ─────────────────────────────────────────
    std::cout << "\n--- Test 1: Basic Log Levels ---\n";
    LOG_DEBUG("Gateway initialising subsystems");
    LOG_INFO ("Reactor started on port 8080");
    LOG_WARN ("High memory usage detected: 82%");
    LOG_ERROR("Client FD 9 authentication REJECTED");

    std::this_thread::sleep_for(50ms); // let consumer drain

    // ── Test 2: Multi-threaded producers ────────────────────────────────
    std::cout << "\n--- Test 2: 4 Threads Logging Concurrently ---\n";
    {
        std::vector<std::thread> producers;
        for (int t = 0; t < 4; ++t) {
            producers.emplace_back([&logger, t]() {
                for (int i = 0; i < 20; ++i) {
                    std::ostringstream oss;
                    oss << "[Thread-" << t << "] packet #" << i << " processed";
                    logger.info(oss.str());
                }
            });
        }
        for (auto& thr : producers) thr.join();
    }

    std::this_thread::sleep_for(100ms); // let consumer drain

    // ── Test 3: High-volume burst — test ring buffer back-pressure ───────
    std::cout << "\n--- Test 3: High-Volume Burst (2000 entries) ---\n";
    {
        int logged = 0, dropped_before = static_cast<int>(logger.dropped_count());
        for (int i = 0; i < 2000; ++i) {
            std::ostringstream oss;
            oss << "Burst entry #" << i;
            if (logger.info(oss.str())) ++logged;
        }
        std::this_thread::sleep_for(500ms); // flush
        int dropped = static_cast<int>(logger.dropped_count()) - dropped_before;
        std::cout << "[+] Burst: " << logged << " queued, "
                  << dropped << " dropped (ring full)\n";
    }

    // ── Test 4: Error path logging ───────────────────────────────────────
    std::cout << "\n--- Test 4: Error Path Logging ---\n";
    LOG_ERROR("CRITICAL: epoll_wait returned -1, errno=EBADF");
    LOG_WARN ("Rate limiter threshold exceeded for 192.168.1.10");
    LOG_INFO ("Connection FD 12 closed cleanly after 320 packets");

    std::this_thread::sleep_for(100ms);

    logger.stop();
    std::cout << "\n[+] Logger stopped. Total dropped entries: "
              << logger.dropped_count() << "\n";
    std::cout << "[+] Phase 14 complete.\n";
    return 0;
}
