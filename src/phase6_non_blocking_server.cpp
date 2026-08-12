#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include "network/SocketUtils.hpp"

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 6: NON-BLOCKING SOCKETS & O_NONBLOCK\n";
    std::cout << "=================================================\n";

    // 1. Create Server Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return 1;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Set Server Socket to NON-BLOCKING mode
    if (!SocketUtils::set_non_blocking(server_fd)) {
        std::cerr << "[-] Failed to set non-blocking mode on server socket\n";
        close(server_fd);
        return 1;
    }
    std::cout << "[+] Server socket (fd: " << server_fd << ") configured with O_NONBLOCK flag.\n";

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) return 1;
    if (listen(server_fd, SOMAXCONN) < 0) return 1;

    std::cout << "[+] Non-Blocking Server listening on port " << PORT << "...\n";
    std::cout << "[+] Entering Non-Blocking Event Loop (Polling mode)...\n";

    std::vector<int> client_fds;
    int loop_counter = 0;

    // Non-blocking polling loop (Precursor to epoll in Phase 7)
    while (loop_counter < 30) { // Run 30 loop iterations for demonstration
        loop_counter++;

        // A. Non-blocking accept() call
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd >= 0) {
            // New connection accepted! Set client socket non-blocking
            SocketUtils::set_non_blocking(client_fd);
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            std::cout << "\n[+] [Loop " << loop_counter << "] Accepted Client! IP: " 
                      << client_ip << ", FD: " << client_fd << "\n";
            client_fds.push_back(client_fd);
        } else if (SocketUtils::is_would_block()) {
            // accept() did NOT block! It immediately returned EAGAIN/EWOULDBLOCK
            // Server can do other work without hanging!
        } else {
            perror("[-] accept error");
        }

        // B. Non-blocking recv() across active client sockets
        char buffer[BUFFER_SIZE];
        for (auto it = client_fds.begin(); it != client_fds.end(); ) {
            int c_fd = *it;
            std::memset(buffer, 0, BUFFER_SIZE);

            ssize_t bytes = recv(c_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes > 0) {
                std::cout << "[+] [Loop " << loop_counter << "] Client (fd " << c_fd << ") sent " 
                          << bytes << " bytes: " << buffer;
                // Echo back
                send(c_fd, buffer, bytes, 0);
                ++it;
            } else if (bytes == 0) {
                std::cout << "[-] Client (fd " << c_fd << ") disconnected.\n";
                close(c_fd);
                it = client_fds.erase(it);
            } else {
                if (SocketUtils::is_would_block()) {
                    // No data available on socket right now (EAGAIN), continue loop
                    ++it;
                } else {
                    perror("[-] recv error");
                    close(c_fd);
                    it = client_fds.erase(it);
                }
            }
        }

        // Sleep briefly to prevent 100% CPU spinning during polling demo
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Cleanup
    for (int c_fd : client_fds) close(c_fd);
    close(server_fd);

    std::cout << "\n=================================================\n";
    std::cout << " PHASE 6 COMPLETE!\n";
    std::cout << "=================================================\n";
    return 0;
}
