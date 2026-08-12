#include <iostream>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

// ─────────────────────────────────────────────────────────────
// PHASE 11: UDP DATAGRAM SUBSYSTEM
//
// What is different from TCP?
//   TCP  — connection-oriented, stream, reliable, ordered
//   UDP  — connectionless, datagram, unreliable, fast
//
// Key syscalls:
//   socket(AF_INET, SOCK_DGRAM, 0)    — UDP socket
//   bind()                            — bind to port (no listen/accept!)
//   recvfrom(fd, buf, len, flags,     — receive + capture sender address
//            &src_addr, &src_len)
//   sendto(fd, buf, len, flags,       — reply to arbitrary address
//          &dst_addr, dst_len)
// ─────────────────────────────────────────────────────────────

constexpr int UDP_PORT    = 9090;
constexpr int MAX_PAYLOAD = 65507; // max UDP payload (65535 - 8 IP hdr - 20 UDP hdr)
constexpr int EPOLL_MAX   = 64;

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 11: UDP DATAGRAM SUBSYSTEM\n";
    std::cout << " (SOCK_DGRAM, recvfrom, sendto, epoll)\n";
    std::cout << "=================================================\n\n";

    // ── Step 1: Create UDP socket ──────────────────────────────
    // SOCK_DGRAM  → connectionless datagrams (UDP)
    // IPPROTO_UDP → explicitly specify UDP (0 also works for DGRAM)
    int udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_fd < 0) {
        perror("[!] socket");
        return 1;
    }
    std::cout << "[+] UDP socket created (fd=" << udp_fd << ")\n";

    // ── Step 2: Allow port reuse (same as TCP) ─────────────────
    int opt = 1;
    setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // ── Step 3: Bind to port — no listen(), no accept()! ───────
    // UDP has NO connection handshake. We simply bind and wait.
    struct sockaddr_in server_addr{};
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(UDP_PORT);

    if (bind(udp_fd, reinterpret_cast<struct sockaddr*>(&server_addr),
             sizeof(server_addr)) < 0) {
        perror("[!] bind");
        close(udp_fd);
        return 1;
    }
    std::cout << "[+] Bound to UDP port " << UDP_PORT << "\n";

    // ── Step 4: epoll to watch UDP socket for readability ───────
    // We can use epoll with UDP just like TCP — whenever a datagram
    // arrives the fd becomes readable.
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        perror("[!] epoll_create1");
        close(udp_fd);
        return 1;
    }

    struct epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = udp_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_fd, &ev) < 0) {
        perror("[!] epoll_ctl");
        close(udp_fd);
        close(epoll_fd);
        return 1;
    }
    std::cout << "[+] epoll watching UDP socket\n";
    std::cout << "[+] Waiting for datagrams (send with: echo 'hello' | nc -u 127.0.0.1 9090)...\n\n";

    // ── Step 5: Event loop ──────────────────────────────────────
    struct epoll_event events[EPOLL_MAX];
    char buf[MAX_PAYLOAD];
    uint64_t total_datagrams = 0;
    uint64_t total_bytes     = 0;

    while (true) {
        int n = epoll_wait(epoll_fd, events, EPOLL_MAX, 5000); // 5-sec timeout
        if (n < 0) {
            perror("[!] epoll_wait");
            break;
        }
        if (n == 0) {
            std::cout << "[~] 5s idle — waiting...\n";
            continue;
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd != udp_fd) continue;

            // ── recvfrom: single syscall gives us BOTH data AND sender ──
            struct sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);

            ssize_t bytes = recvfrom(
                udp_fd, buf, sizeof(buf) - 1, 0,
                reinterpret_cast<struct sockaddr*>(&client_addr),
                &client_len
            );

            if (bytes < 0) {
                perror("[!] recvfrom");
                continue;
            }
            buf[bytes] = '\0'; // null-terminate for display

            // Extract sender IP and port
            char sender_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);
            uint16_t sender_port = ntohs(client_addr.sin_port);

            total_datagrams++;
            total_bytes += static_cast<uint64_t>(bytes);

            std::cout << "[DATAGRAM #" << total_datagrams << "] "
                      << "From " << sender_ip << ":" << sender_port
                      << "  Size=" << bytes << " B"
                      << "  Payload: \"" << buf << "\"\n";

            // ── sendto: echo the datagram back to the exact sender ──
            // No connection needed — we specify destination on every send.
            ssize_t sent = sendto(
                udp_fd, buf, static_cast<size_t>(bytes), 0,
                reinterpret_cast<struct sockaddr*>(&client_addr),
                client_len
            );
            if (sent < 0) {
                perror("[!] sendto");
            } else {
                std::cout << "  [ECHO] Sent " << sent << " B back to "
                          << sender_ip << ":" << sender_port << "\n";
            }

            // ── Key stat: UDP has no partial reads. ─────────────
            // recvfrom always delivers exactly one datagram atomically.
            // If your buf was too small, the rest is SILENTLY DROPPED.
            if (bytes == static_cast<ssize_t>(sizeof(buf) - 1)) {
                std::cout << "  [WARN] Buffer full — datagram may have been truncated!\n";
            }
        }
    }

    std::cout << "\n[+] Summary: " << total_datagrams << " datagrams, "
              << total_bytes << " bytes total\n";

    close(epoll_fd);
    close(udp_fd);
    return 0;
}
