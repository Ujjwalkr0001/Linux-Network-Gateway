# High-Performance C++ Linux Network Gateway — Implementation Plan & Roadmap

## Overview & Objective

This document outlines the architectural roadmap and phase-by-phase implementation plan for building a **High-Performance C++ Linux Network Gateway**. The project is designed specifically to prepare for a **Qualcomm Software Engineering Intern role**, focusing heavily on systems programming, modern C++ (C++17/C++20), POSIX networking APIs, Linux kernel & OS fundamentals, multithreading & concurrency, event-driven architectures (`epoll`), routing algorithms, security, benchmarking, and debugging tools (`GDB`, `Valgrind`, `ASan`).

The gateway will evolve progressively from a basic blocking TCP socket server into an event-driven, multithreaded network gateway capable of handling thousands of concurrent connections using edge-triggered `epoll`, thread pools, custom binary packet framing, routing engines, token bucket rate limiting, telemetry dashboards, and an optional educational Linux kernel module.

---

## User Review Required

> [!IMPORTANT]
> **Environment Note (Windows vs Linux Target)**:
> Since your local system is running **Windows**, native Linux POSIX header files (`<sys/socket.h>`, `<sys/epoll.h>`, `<netinet/in.h>`, `<arpa/inet.h>`) and Linux Kernel headers are not natively available under Windows MSVC/WinSock without Linux compatibility layers.
> 
> **Recommended Setup Options for Phase 0:**
> 1. **WSL2 (Windows Subsystem for Linux - Ubuntu 22.04 or 24.04 LTS)** — *Recommended*: Provides a genuine Linux kernel environment, full POSIX socket support, `epoll`, GDB, Valgrind, and Kernel module compilation capabilities inside VS Code.
> 2. **Docker Container / Remote Linux VM**: Provides an isolated Linux development environment.

> [!NOTE]
> **Pedagogical Constraints & Incremental Workflow**:
> - We will **NOT** dump full codebase files at once.
> - Each phase will be presented step-by-step: core OS/networking concepts, architecture diagrams, code walk-throughs, compilation commands, test scripts, empirical output analysis, and Qualcomm interview relevant deep-dives.
> - Execution proceeds to the next phase ONLY after you have compiled, tested, verified, and approved the current phase.

---

## Open Questions

1. **Preferred Linux Environment for Windows**: Are you currently using **WSL2 (Ubuntu)** inside VS Code, or would you like guidance setting up WSL2 / Docker for this project?
2. **C++ Standard Preference**: Should we set `C++17` as our baseline (widely used in Qualcomm embedded/systems codebases) or `C++20` (leveraging concepts and `std::jthread`)? We recommend **C++17** for broad compatibility while selectively using modern idioms.

---

## Complete 25-Phase Master Roadmap

- [x] **Phase 0**: Environment & Toolchain Setup (WSL2, GCC, CMake, GDB, Valgrind, `/proc`)
- [x] **Phase 1**: Simple TCP Server (`socket`, `bind`, `listen`, `accept`, `recv`, `send`, `close`)
- [x] **Phase 2**: TCP Echo Server & Buffer Lifecycle (Blocking I/O, Kernel socket buffers)
- [x] **Phase 3**: Multi-Client Concurrency (Thread-per-Client, `std::thread`, context cost)
- [x] **Phase 4**: Production-Grade Thread Pool Engine (Task Queue, Mutex, Condition Variable, Shutdown)
- [x] **Phase 5**: Custom Binary Protocol Parser (Fixed Header, Endianness, Framing)
- [x] **Phase 6**: Non-Blocking Sockets & `O_NONBLOCK` (`EAGAIN`, `EWOULDBLOCK` management)
- [x] **Phase 7**: Linux `epoll` Deep Dive (`epoll_create1`, `epoll_ctl`, `epoll_wait`, `EPOLLET`)
- [x] **Phase 8**: Scalable Event-Driven Reactor Core (Listener & I/O event routing)
- [x] **Phase 9**: State-Machine Connection Manager (State transitions, ring buffers)
- [x] **Phase 10**: High-Performance Routing Engine (CIDR prefix matching, Trie lookup)
- [x] **Phase 11**: Datagram Subsystem / UDP Listener (`sendto`, `recvfrom`, UDP vs TCP)
- [x] **Phase 12**: Concurrency-Safe Rate Limiter (Token bucket algorithm, `std::atomic`)
- [x] **Phase 13**: Session Authentication System (AUTH state machine & handshakes)
- [ ] **Phase 14**: Asynchronous Low-Overhead Logger (Ring buffer logging subsystem)
- [ ] **Phase 15**: Telemetry & Live CLI Dashboard (Throughput, dropped packets, latencies)
- [ ] **Phase 16**: Empirical Benchmarking Suite (Comparative analysis across Phase 3/4/8)
- [ ] **Phase 17**: Gateway Stress Tester & Fuzzer (Malformed inputs, slowloris simulation)
- [ ] **Phase 18**: System Call & Resilience Audit (Partial write handling, socket resets)
- [ ] **Phase 19**: Dynamic Analysis & Sanitizers (ASan, UBSan, TSan, GDB core dump analysis)
- [ ] **Phase 20**: Automated Unit Testing Suite (GoogleTest setup for parsers & routing)
- [ ] **Phase 21**: Configuration Subsystem (File-based runtime gateway config)
- [ ] **Phase 22**: Security Audit & Threat Modeling (DoS vectors, educational vs prod)
- [ ] **Phase 23**: Educational Linux Kernel Module (Kernel vs user space, procfs/sysfs)
- [ ] **Phase 24**: Packet Capture Subsystem (Raw socket / libpcap dissection)
- [ ] **Phase 25**: Codebase Modularization & Production Packaging (CMake & Final README)

---

## Detailed Phase Breakdown

### - [x] Phase 0: Environment Setup & Linux Systems Toolchain Verification
- **Concepts**: Linux process layout, virtual memory spaces, `/proc` filesystem, GCC compiler flags (`-Wall -Wextra -O2 -g`), CMake targets, GDB workflow, Valgrind memory tracking.
- **Deliverables**: Verification script, environment check commands, setup of workspace CMake structure.

### - [x] Phase 1: Simple Single-Client TCP Server
- **Concepts**: POSIX sockets, File Descriptor table, socket lifecycle (`socket()`, `bind()`, `listen()`, `accept()`, `recv()`, `send()`, `close()`), TCP 3-way handshake under the hood.
- **Deliverables**: Single-file minimalist C++ TCP server responding to a netcat client.

### - [x] Phase 2: TCP Echo Server & Socket Buffer Mechanics
- **Concepts**: Blocking I/O mechanics, kernel `sk_buff` structures, socket send/receive buffers, partial reads/writes, TCP stream framing (lack of message boundaries).
- **Deliverables**: Echo server handling arbitrary stream chunking safely.

### - [x] Phase 3: Multi-Client Concurrency (Thread-per-Connection)
- **Concepts**: Processes vs Threads, kernel thread scheduling, stack memory cost (8MB default limit per thread), thread context switching latency, race conditions.
- **Deliverables**: Server spawning `std::thread` per accepted connection.

### - [x] Phase 4: Production-Grade Thread Pool Implementation
- **Concepts**: Thread starvation, C++17 `std::mutex`, `std::unique_lock`, `std::condition_variable`, Producer-Consumer queue pattern, graceful worker shutdown & joining.
- **Deliverables**: Reusable C++ ThreadPool class replacing unbounded thread creation.

### - [x] Phase 5: Custom Binary Application Protocol Engine
- **Concepts**: Serialization, byte order / endianness (`htons`, `htonl`, `ntohs`, `ntohl`), packet framing, header vs payload separation, corrupt length attack prevention.
- **Header Spec**: `[ Version (1B) | Type (1B) | Length (4B) | Payload (NB) ]`.
- **Deliverables**: Binary packet encoder, decoder, and validator with full unit test cases.

### - [x] Phase 6: Non-Blocking Socket Mechanics
- **Concepts**: Synchronous vs Asynchronous I/O, `fcntl(fd, F_SETFL, O_NONBLOCK)`, `EAGAIN` and `EWOULDBLOCK` status codes, busy polling pitfalls.
- **Deliverables**: Server converting client and listening sockets to non-blocking mode.

### - [x] Phase 7: Linux `epoll` Core Architecture
- **Concepts**: `select()` / `poll()` $O(N)$ limitations vs `epoll` $O(1)$ readiness notification, kernel red-black tree & ready list, `epoll_create1()`, `epoll_ctl()`, `epoll_wait()`, Level-Triggered (LT) vs Edge-Triggered (ET `EPOLLET`).
- **Deliverables**: Deep-dive epoll tutorial script demonstrating event firing under LT vs ET.

### - [x] Phase 8: Event-Driven Scalable Reactor Core
- **Concepts**: Reactor pattern, event loop dispatching, handling partial socket reads/writes non-blockingly, connection teardown on client disconnect / EOF (`EPOLLHUP`, `EPOLLERR`).
- **Deliverables**: Fully modular EventLoop / EpollReactor class driving network I/O.

### - [ ] Phase 9: State-Machine Connection Manager
- **Concepts**: Session state management (`CONNECTED` -> `AUTHENTICATING` -> `AUTHENTICATED` -> `CLOSING`), per-connection read/write ring buffers, idle timeout management.
- **Deliverables**: `ConnectionManager` class encapsulating active socket context and metadata.

### - [ ] Phase 10: High-Performance Routing Engine
- **Concepts**: IPv4 CIDR notation, subnet masks, Longest Prefix Match (LPM) algorithm, Trie / Radix Tree data structures vs linear lookup tables, routing lookup complexity.
- **Deliverables**: Forwarding engine matching packet destination IPs to gateway output interfaces with nanosecond benchmarking.

### - [ ] Phase 11: Datagram Subsystem / UDP Support
- **Concepts**: Connectionless vs connection-oriented transport, IP fragmentation, UDP socket API (`sendto()`, `recvfrom()`), multiplexing UDP inside epoll loops.
- **Deliverables**: Dual-stack TCP/UDP gateway receiving datagram traffic.

### - [ ] Phase 12: Concurrency-Safe Rate Limiter
- **Concepts**: Token Bucket & Leaky Bucket algorithms, `std::atomic` operations, memory order constraints (`std::memory_order_relaxed` / `acq_rel`), Denial of Service (DoS) mitigation.
- **Deliverables**: Thread-safe RateLimiter class enforcing per-IP packet/second quotas.

### - [ ] Phase 13: Session Authentication Subsystem
- **Concepts**: Protocol handshake state transitions, challenge-response mechanism, handling unauthenticated client isolation.
- **Deliverables**: `AUTH_REQUEST` and `AUTH_RESPONSE` binary protocol handlers.

### - [ ] Phase 14: Asynchronous Low-Overhead Logging System
- **Concepts**: I/O bottleneck in logging, lock-free ring buffers, dedicated background logging worker thread, log levels (`DEBUG`, `INFO`, `WARN`, `ERROR`), ISO 8601 timestamps.
- **Deliverables**: High-throughput non-blocking logger for gateway diagnostics.

### - [ ] Phase 15: Real-Time Telemetry & Live CLI Dashboard
- **Concepts**: Performance counter collection, CPU usage reading via `/proc/self/stat`, Resident Set Size (RSS) memory tracking, terminal formatting (ANSI escape codes).
- **Deliverables**: Live updating terminal UI displaying active connections, throughput (MB/s), latency, and packet drops.

### - [ ] Phase 16: Empirical Performance Benchmarking Suite
- **Concepts**: Micro-benchmarking vs end-to-end benchmarking, latency percentile measurement ($P_{50}$, $P_{99}$), measuring performance degradation under high connection scale.
- **Deliverables**: Benchmarking tool measuring throughput and latency across Thread-per-Client (Phase 3), Thread Pool (Phase 4), and Epoll Reactor (Phase 8).

### - [ ] Phase 17: Gateway Stress Tester & Fuzzing Tool
- **Concepts**: Robustness testing, TCP SYN flooding, Slowloris attack simulation, malformed binary packet fuzzing, connection churn testing.
- **Deliverables**: Custom stress test suite ensuring zero crashes or memory corruption under attack.

### - [ ] Phase 18: System Call & Memory Resilience Audit
- **Concepts**: Defensive programming, errno handling (`EINTR`, `EPIPE`, `ECONNRESET`), stack allocation safety, memory allocation failure guards.
- **Deliverables**: System-wide error handling audit fixing socket leakage edges.

### - [ ] Phase 19: Dynamic Analysis & Sanitizer Verification
- **Concepts**: Compiler instrumentation, AddressSanitizer (`-fsanitize=address`), UndefinedBehaviorSanitizer (`-fsanitize=undefined`), ThreadSanitizer (`-fsanitize=thread`), Valgrind Memcheck & Helgrind, GDB backtrace analysis.
- **Deliverables**: Clean sanitizer execution report with zero leaks, races, or UB.

### - [ ] Phase 20: Automated GoogleTest Unit Testing Suite
- **Concepts**: Unit testing best practices, test fixtures, mocking I/O interfaces, test-driven validation.
- **Deliverables**: Automated `ctest` setup testing binary serialization, routing lookup, and rate limiting logic.

### - [ ] Phase 21: Runtime Configuration Subsystem
- **Concepts**: Configuration file parsing, decoupling build parameters from runtime parameters, dynamic hot-reloading concepts.
- **Deliverables**: Config loader supporting port, thread pool size, rate limits, and log levels.

### - [ ] Phase 22: Security Audit & Threat Modeling
- **Concepts**: Attack surface reduction, buffer boundary enforcement, Resource Exhaustion DoS analysis, educational vs production-grade security gaps.
- **Deliverables**: Comprehensive security threat matrix document.

### - [ ] Phase 23: Educational Linux Kernel Module (Optional Extension)
- **Concepts**: User space vs Kernel space ring separation (Ring 3 vs Ring 0), kernel module entry/exit (`init_module`/`cleanup_module`), character devices, `procfs`/`sysfs` driver interfaces, kernel logging (`printk`).
- **Deliverables**: Minimal custom Linux kernel module exposing kernel-level gateway packet statistics.

### - [ ] Phase 24: Packet Capture & Dissection Subsystem (Optional Extension)
- **Concepts**: Promiscuous mode, Linux raw sockets (`AF_PACKET`), `libpcap` integration, BPF (Berkeley Packet Filters), Wireshark equivalence.
- **Deliverables**: Raw packet sniffer tool parsing Ethernet/IP/TCP headers.

### - [ ] Phase 25: Final Architecture Refactoring & Production Packaging
- **Concepts**: Clean modular folder structure, modern CMake target exports, build types (`Release`, `Debug`, `RelWithDebInfo`), root `README.md` portfolio documentation.
- **Deliverables**: Fully structured build tree ready for GitHub portfolio presentation.

---

## Directory Structure Plan

Upon full completion, the workspace will follow standard C++ enterprise conventions:

```
cpp-network-gateway/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── Architecture.md
│   ├── BinaryProtocol.md
│   └── Benchmarks.md
├── include/
│   ├── gateway/
│   │   ├── core/           # EventLoop, EpollReactor
│   │   ├── network/        # Socket, IPv4Address, Connection
│   │   ├── protocol/       # Packet, PacketParser, Endian
│   │   ├── concurrency/   # ThreadPool, TaskQueue, Mutex
│   │   ├── routing/        # RoutingTable, CIDRNode
│   │   ├── security/       # RateLimiter, Authenticator
│   │   ├── monitoring/     # Metrics, TelemetryDashboard
│   │   ├── logging/        # AsyncLogger
│   │   └── config/         # ConfigParser
├── src/
│   ├── core/
│   ├── network/
│   ├── protocol/
│   ├── concurrency/
│   ├── routing/
│   ├── security/
│   ├── monitoring/
│   ├── logging/
│   ├── config/
│   └── main.cpp
├── tests/
│   ├── unit/               # GoogleTest unit tests
│   └── integration/
├── benchmarks/
│   └── throughput_benchmark.cpp
├── tools/
│   ├── stress_tester.cpp
│   └── client_simulator.py
└── kernel/
    ├── Makefile
    └── gateway_stats_mod.c
```

---

## Phase 0 Initial Verification Task (To Be Executed First)

Before writing any gateway networking code, we will perform **Phase 0 Environment Setup & Verification**:

1. **Verify Linux Environment**: Ensure GCC (g++ standard C++17 support), CMake (3.15+), GDB, and Valgrind are installed.
2. **Linux System Inspection**: Practice inspecting running processes, memory limits, and open file descriptors via `/proc`:
   - `cat /proc/sys/net/ipv4/ip_local_port_range`
   - `ulimit -n` (Max open file descriptors, critical for high-connection gateways)
   - `cat /proc/cpuinfo`
3. **Compile Test Script**: Compile a simple multi-threaded C++ system check binary with `-Wall -Wextra -std=c++17` to verify toolchain operation.

---

## Verification Plan

### Automated Verification
- CMake build target verification: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build`
- Sanitizer checks: Run compiled binaries compiled with `-fsanitize=address,undefined`.
- Unit tests: Automated execution via `ctest --output-on-failure`.

### Manual & Interactive Verification
- Socket testing: `nc -zv localhost 8080`, `netcat`, `curl`, and custom client scripts.
- Packet inspection: `tcpdump -i lo port 8080 -X` to inspect raw binary bytes over localhost.
- System metrics: `top`, `htop`, `/proc/<pid>/status`, and `lsof -p <pid>`.