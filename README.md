# 🌐 High-Performance C++ Linux Network Gateway

A **25-phase, production-quality educational C++ project** implementing a complete Linux network gateway from raw TCP sockets through to kernel modules — covering every layer of systems programming from user-space to Ring 0.

> **Built for**: Deep mastery of Linux systems programming, C++17, POSIX sockets, concurrency, networking protocols, and OS internals.

---

## 📐 Architecture Overview

```
┌──────────────────────────────────────────────────────────────────────────┐
│                       Linux Network Gateway                              │
│  ┌─────────────────┐  ┌──────────────────┐  ┌──────────────────────┐    │
│  │  Client Layer   │  │  Protocol Engine  │  │   Routing Engine     │    │
│  │  TCP / UDP      │→ │  Binary Protocol  │→ │   CIDR Trie LPM     │    │
│  │  Raw Sockets    │  │  Packet.hpp       │  │   RouteTable.hpp    │    │
│  └─────────────────┘  └──────────────────┘  └──────────────────────┘    │
│                                  │                                        │
│  ┌─────────────────┐  ┌──────────┴───────┐  ┌──────────────────────┐    │
│  │  Auth System    │  │   Epoll Reactor   │  │   Rate Limiter       │    │
│  │  Challenge-     │  │   Event-Driven    │  │   Token Bucket       │    │
│  │  Response Nonce │  │   I/O Multiplexer │  │   atomic CAS         │    │
│  └─────────────────┘  └──────────────────┘  └──────────────────────┘    │
│                                                                           │
│  ┌─────────────────┐  ┌──────────────────┐  ┌──────────────────────┐    │
│  │  Async Logger   │  │   Telemetry      │  │   Config Loader      │    │
│  │  Ring Buffer    │  │   Live CLI HUD   │  │   gateway.conf       │    │
│  │  Lock-Free      │  │   /proc/self/stat│  │   Hot-Reload         │    │
│  └─────────────────┘  └──────────────────┘  └──────────────────────┘    │
│                                                                           │
│  ┌─────────────────────────────────────────────────────────────────┐     │
│  │ Kernel Module (Ring 0)  /proc/gateway_stats  ← LKM Driver      │     │
│  └─────────────────────────────────────────────────────────────────┘     │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Build & Run

### Prerequisites
- **Linux OS** (Ubuntu 22.04+ / Debian 12+ recommended)
- **g++ 11+** with C++17 support
- **CMake 3.15+**
- **Linux kernel headers** (for Phase 23 kernel module only)

### Build All Targets

```bash
# Clone repository
git clone https://github.com/Ujjwalkr0001/Linux-Network-Gateway.git
cd Linux-Network-Gateway

# Debug build (default)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build (optimized)
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release

# With AddressSanitizer
cmake -B build_asan -DENABLE_ASAN=ON
cmake --build build_asan
```

### Run Tests (CTest)
```bash
ctest --test-dir build --output-on-failure
```

---

## 📋 Phase Index — All 25 Phases

| Phase | Title | Binary | Key Concept |
|:-----:|:------|:-------|:------------|
| 0 | Environment Setup | `env_check` | Linux toolchain verification |
| 1 | TCP Server Fundamentals | `phase1_tcp_server` | `socket`, `bind`, `listen`, `accept` |
| 2 | Echo Server | `phase2_tcp_echo_server` | Full-duplex `send`/`recv` loop |
| 3 | Multi-Threaded Server | `phase3_multithreaded_server` | Thread-per-client, `std::thread` |
| 4 | Thread Pool Engine | `phase4_threadpool_server` | Task queue, mutex, condition variable |
| 5 | Binary Protocol Parser | `phase5_binary_protocol` | Fixed header, endianness, framing |
| 6 | Non-Blocking Sockets | `phase6_non_blocking_server` | `O_NONBLOCK`, `EAGAIN`, `EWOULDBLOCK` |
| 7 | Linux `epoll` Deep Dive | `phase7_epoll_demo` | `epoll_create1`, `epoll_ctl`, `EPOLLET` |
| 8 | Epoll Reactor Core | `phase8_reactor_server` | Event-driven I/O reactor pattern |
| 9 | Connection Manager | `phase9_connection_manager_server` | State machine, idle timeouts |
| 10 | CIDR Routing Engine | `phase10_routing_engine` | Binary Trie, Longest Prefix Match |
| 11 | UDP Datagram Subsystem | `phase11_udp_server` | `sendto`, `recvfrom`, UDP vs TCP |
| 12 | Token Bucket Rate Limiter | `phase12_rate_limiter` | `std::atomic`, CAS, lock-free |
| 13 | Session Authentication | `phase13_auth_system` | Challenge-response, nonce, HMAC |
| 14 | Async Ring Buffer Logger | `phase14_async_logger` | Lock-free ring buffer, background thread |
| 15 | Telemetry & Live Dashboard | `phase15_telemetry_dashboard` | `/proc/self/stat`, ANSI terminal UI |
| 16 | Benchmarking Suite | `phase16_benchmarking_suite` | P50/P90/P99 latency percentiles |
| 17 | Stress Tester & Fuzzer | `phase17_stress_fuzzer` | Malformed packets, Slowloris, connection churn |
| 18 | Resilience Audit | `phase18_resilience_audit` | `EINTR`, `MSG_NOSIGNAL`, RAII sockets |
| 19 | Dynamic Analysis | `phase19_sanitizers_demo` | ASan, TSan, UBSan, Valgrind, GDB |
| 20 | Unit Testing Suite | `phase20_unit_tests` | `ctest` harness, assertion macros |
| 21 | Config Subsystem | `phase21_config_subsystem` | File-based config, hot-reload |
| 22 | Security Audit | `phase22_security_audit` | STRIDE threat model, DoS mitigations |
| 23 | Linux Kernel Module | `phase23_kernel_module_demo` | Ring 0 LKM, procfs, `printk` |
| 24 | Packet Capture | `phase24_packet_capture` | `AF_PACKET`, Ethernet/IP/TCP/UDP dissection |
| 25 | Production Packaging | — | CMake build types, README, portfolio |

---

## 📁 Project Structure

```
Linux-Network-Gateway/
├── CMakeLists.txt            # CMake build system (25 executables)
├── ImplementationPlan.md     # 25-phase master roadmap
├── README.md                 # This file
├── config/
│   └── gateway.conf          # Runtime configuration file
├── docs/
│   └── SecurityAudit.md      # STRIDE threat model & security audit
├── include/
│   ├── auth/AuthSession.hpp          # Challenge-response auth
│   ├── benchmark/BenchmarkingSuite.hpp
│   ├── config/ConfigParser.hpp       # Runtime config loader
│   ├── core/EpollReactor.hpp         # Epoll event reactor
│   ├── core/ThreadPool.hpp           # Thread pool engine
│   ├── logging/AsyncLogger.hpp       # Lock-free ring buffer logger
│   ├── network/ConnectionManager.hpp
│   ├── network/PacketDissector.hpp   # L2/L3/L4 dissector
│   ├── network/RAIISocket.hpp        # Move-only RAII socket wrapper
│   ├── protocol/Packet.hpp           # Binary packet protocol
│   ├── ratelimit/RateLimiter.hpp     # Token bucket (atomic CAS)
│   ├── routing/RouteTable.hpp        # Binary Trie CIDR routing
│   └── telemetry/Telemetry.hpp       # System metrics collector
├── kernel/
│   ├── gateway_stats_mod.c   # Linux Kernel Module source
│   └── Makefile              # Kbuild compilation Makefile
├── src/
│   ├── phase1_tcp_server.cpp
│   ├── phase2_tcp_echo_server.cpp
│   ├── ... (phases 3–24)
│   └── phase24_packet_capture.cpp
└── tools/
    └── env_check.cpp         # Phase 0 environment validator
```

---

## 🔑 Key Systems Programming Concepts Covered

| Domain | Concepts |
|:-------|:---------|
| **Networking** | TCP/UDP sockets, `bind`/`listen`/`accept`, `sendto`/`recvfrom`, non-blocking I/O |
| **Concurrency** | `std::thread`, thread pools, mutexes, condition variables, `std::atomic` CAS |
| **I/O Multiplexing** | `epoll_create1`, `epoll_ctl`, `epoll_wait`, edge-triggered (`EPOLLET`) |
| **Protocol Engineering** | Binary framing, network byte order (`htonl`/`ntohl`), `#pragma pack` |
| **Routing** | CIDR notation, Binary Trie, Longest Prefix Match (LPM) |
| **Security** | STRIDE threat model, replay attack prevention, rate limiting, HMAC |
| **Memory Safety** | RAII, move semantics, `std::unique_ptr`, ASan/TSan/UBSan |
| **Kernel Programming** | LKM (`insmod`/`rmmod`), procfs, `atomic64_t`, `printk`, Ring 0 vs Ring 3 |
| **Packet Analysis** | Ethernet II, IPv4, TCP, UDP header dissection, raw `AF_PACKET` sockets |

---

## 📜 License

Educational project — MIT License.