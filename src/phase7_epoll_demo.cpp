#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "network/SocketUtils.hpp"

constexpr int PORT = 8080;
constexpr int MAX_EVENTS = 64;
constexpr int BUFFER_SIZE = 1024;

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 7: LINUX EPOLL DEEP DIVE\n";
    std::cout << "=================================================\n";

    // 1. Create Listening Socket & set Non-Blocking
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return 1;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    SocketUtils::set_non_blocking(server_fd);

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) return 1;
    if (listen(server_fd, SOMAXCONN) < 0) return 1;

    // 2. Create epoll instance in Linux Kernel
    // epoll_create1(EPOLL_CLOEXEC) creates a Red-Black Tree in kernel memory to track FDs
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        perror("[-] epoll_create1 failed");
        close(server_fd);
        return 1;
    }
    std::cout << "[+] Created epoll instance. Epoll File Descriptor (epoll_fd): " << epoll_fd << "\n";

    // 3. Register server_fd with epoll instance (EPOLL_CTL_ADD)
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN; // Monitor incoming read/connection events
    ev.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("[-] epoll_ctl EPOLL_CTL_ADD failed for server_fd");
        close(epoll_fd);
        close(server_fd);
        return 1;
    }
    std::cout << "[+] Registered listening socket (fd: " << server_fd << ") into epoll interest list.\n";
    std::cout << "[+] Server listening on port " << PORT << ". Waiting for epoll_wait() events...\n\n";

    struct epoll_event events[MAX_EVENTS];

    // 4. Main Event Loop
    int event_loop_runs = 0;
    while (event_loop_runs < 20) { // Run for 20 event notifications
        // epoll_wait BLOCKS until at least one registered socket is ready for I/O!
        // Consumes 0% CPU while waiting in kernel!
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // -1 = infinite timeout
        if (nfds < 0) {
            perror("[-] epoll_wait error");
            break;
        }

        event_loop_runs++;
        std::cout << "[+] [epoll_wait #" << event_loop_runs << "] Kernel returned " << nfds << " ready event(s):\n";

        for (int i = 0; i < nfds; ++i) {
            int current_fd = events[i].data.fd;
            uint32_t current_events = events[i].events;

            // A. Event on Listening Socket -> Accept new client connection
            if (current_fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

                if (client_fd >= 0) {
                    SocketUtils::set_non_blocking(client_fd);

                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                    std::cout << "  - [Accept Event] New Client IP: " << client_ip 
                              << ", FD: " << client_fd << "\n";

                    // Register new client_fd into epoll with Level-Triggered EPOLLIN
                    struct epoll_event client_ev;
                    std::memset(&client_ev, 0, sizeof(client_ev));
                    client_ev.events = EPOLLIN; // Level-Triggered mode
                    client_ev.data.fd = client_fd;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0) {
                        perror("[-] epoll_ctl client_fd failed");
                        close(client_fd);
                    }
                }
            } 
            // B. Event on Client Socket -> Read incoming data payload
            else if (current_events & EPOLLIN) {
                char buffer[BUFFER_SIZE];
                std::memset(buffer, 0, BUFFER_SIZE);

                ssize_t bytes = recv(current_fd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes > 0) {
                    std::cout << "  - [Read Event on FD " << current_fd << "] Received " 
                              << bytes << " bytes: " << buffer;
                    send(current_fd, buffer, bytes, 0); // Echo back
                } else if (bytes == 0) {
                    // Client disconnected (TCP FIN)
                    std::cout << "  - [Disconnect Event] Client on FD " << current_fd << " closed connection.\n";
                    // Remove from epoll interest list & close socket
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
                    close(current_fd);
                } else {
                    if (!SocketUtils::is_would_block()) {
                        perror("[-] recv error");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
                        close(current_fd);
                    }
                }
            }
        }
    }

    close(epoll_fd);
    close(server_fd);
    return 0;
}
