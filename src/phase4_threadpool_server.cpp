#include <iostream>
#include <string>
#include <cstring>
#include <atomic>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "concurrency/ThreadPool.hpp"

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
constexpr size_t THREAD_POOL_SIZE = 4; // Fixed 4 worker threads

std::atomic<uint64_t> active_clients{0};

ssize_t send_all(int socket_fd, const char* buffer, size_t length) {
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t bytes_sent = send(socket_fd, buffer + total_sent, length - total_sent, 0);
        if (bytes_sent <= 0) return bytes_sent;
        total_sent += bytes_sent;
    }
    return total_sent;
}

void handle_client(int client_fd, std::string client_ip, uint16_t client_port) {
    active_clients++;
    std::cout << "[+] [Thread " << std::this_thread::get_id() << "] Processing Client: " 
              << client_ip << ":" << client_port << " (Active Clients: " << active_clients << ")\n";

    char buffer[BUFFER_SIZE];
    while (true) {
        std::memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);

        if (bytes_received > 0) {
            if (std::strncmp(buffer, "quit", 4) == 0 || std::strncmp(buffer, "exit", 4) == 0) {
                break;
            }
            if (send_all(client_fd, buffer, bytes_received) <= 0) break;
        } else {
            break;
        }
    }

    close(client_fd);
    active_clients--;
    std::cout << "[-] [Thread " << std::this_thread::get_id() << "] Client Disconnected: " 
              << client_ip << ":" << client_port << " (Remaining Active: " << active_clients << ")\n";
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 4: PRODUCTION-GRADE THREAD POOL GATEWAY\n";
    std::cout << "=================================================\n";

    // Initialize Thread Pool with fixed N worker threads
    ThreadPool pool(THREAD_POOL_SIZE);
    std::cout << "[+] Initialized ThreadPool with " << pool.worker_count() << " worker threads.\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return 1;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) return 1;
    if (listen(server_fd, SOMAXCONN) < 0) return 1;

    std::cout << "[+] ThreadPool TCP Server listening on port " << PORT << "...\n";

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        uint16_t client_port = ntohs(client_addr.sin_port);

        // Enqueue client connection task into thread pool queue instead of spawning a new OS thread!
        pool.enqueue(handle_client, client_fd, std::string(client_ip), client_port);
    }

    close(server_fd);
    return 0;
}
