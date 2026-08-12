#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

// Helper function to handle robust full writes (handles partial send calls)
ssize_t send_all(int socket_fd, const char* buffer, size_t length) {
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t bytes_sent = send(socket_fd, buffer + total_sent, length - total_sent, 0);
        if (bytes_sent <= 0) {
            if (bytes_sent < 0) {
                perror("[-] send failed in send_all");
            }
            return bytes_sent; // Return error or 0
        }
        total_sent += bytes_sent;
    }
    return total_sent;
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 2: TCP ECHO SERVER & BUFFER LIFECYCLE\n";
    std::cout << "=================================================\n";

    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[-] Socket creation failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Bind address
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        perror("[-] Bind failed");
        close(server_fd);
        return 1;
    }

    // 3. Listen
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("[-] Listen failed");
        close(server_fd);
        return 1;
    }
    std::cout << "[+] Echo Server listening on port " << PORT << "...\n";

    // 4. Accept single connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0) {
        perror("[-] Accept failed");
        close(server_fd);
        return 1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    std::cout << "[+] Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) 
              << " (Client FD: " << client_fd << ")\n";

    // 5. Echo Loop: Continuously recv() and send_all() back to the client
    char buffer[BUFFER_SIZE];
    uint64_t total_bytes_echoed = 0;

    while (true) {
        std::memset(buffer, 0, BUFFER_SIZE);
        
        // recv() blocks until client sends data or closes connection
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);

        if (bytes_received > 0) {
            total_bytes_echoed += bytes_received;
            std::cout << "[+] Received " << bytes_received << " bytes: " << buffer;
            if (buffer[bytes_received - 1] != '\n') std::cout << "\n";

            // Check for exit command from client
            if (std::strncmp(buffer, "quit", 4) == 0 || std::strncmp(buffer, "exit", 4) == 0) {
                std::cout << "[+] Client requested session termination.\n";
                break;
            }

            // Echo the received bytes back to the client using robust partial write handler
            ssize_t bytes_sent = send_all(client_fd, buffer, bytes_received);
            if (bytes_sent <= 0) {
                std::cout << "[-] Failed to echo data back to client.\n";
                break;
            }
            std::cout << "  -> Echoed " << bytes_sent << " bytes back to client.\n";

        } else if (bytes_received == 0) {
            // bytes_received == 0 indicates TCP EOF (Client closed socket with FIN packet)
            std::cout << "[+] Client cleanly disconnected (EOF / TCP FIN received).\n";
            break;
        } else {
            perror("[-] recv error");
            break;
        }
    }

    std::cout << "[+] Connection closed. Total bytes echoed: " << total_bytes_echoed << " bytes.\n";

    close(client_fd);
    close(server_fd);

    std::cout << "=================================================\n";
    std::cout << " PHASE 2 COMPLETE!\n";
    std::cout << "=================================================\n";
    return 0;
}
