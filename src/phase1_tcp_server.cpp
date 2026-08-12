#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 1: SIMPLE SINGLE-CLIENT TCP SERVER\n";
    std::cout << "=================================================\n";

    // 1. Create IPv4 TCP Socket
    // AF_INET = IPv4, SOCK_STREAM = TCP stream socket, 0 = default protocol (IPPROTO_TCP)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[-] Socket creation failed");
        return 1;
    }
    std::cout << "[+] Server socket created successfully. File Descriptor (fd): " << server_fd << "\n";

    // Allow quick port reuse after server restarts (SO_REUSEADDR)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[-] setsockopt(SO_REUSEADDR) failed");
        close(server_fd);
        return 1;
    }

    // 2. Configure Socket Address Structure (sockaddr_in)
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;                 // IPv4 family
    server_addr.sin_addr.s_addr = INADDR_ANY;          // Bind to all local network interfaces (0.0.0.0)
    server_addr.sin_port = htons(PORT);               // Convert port from host byte order to network byte order (Big-Endian)

    // 3. Bind socket to IP address and Port
    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        perror("[-] Bind failed");
        close(server_fd);
        return 1;
    }
    std::cout << "[+] Bound successfully to 0.0.0.0:" << PORT << "\n";

    // 4. Mark socket as passive listener
    // SOMAXCONN = OS kernel backlog queue limit for pending connections
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("[-] Listen failed");
        close(server_fd);
        return 1;
    }
    std::cout << "[+] Server is listening for incoming TCP connections on port " << PORT << "...\n";

    // 5. Accept an incoming client connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // accept() BLOCKS until a client completes the TCP 3-Way Handshake
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0) {
        perror("[-] Accept failed");
        close(server_fd);
        return 1;
    }

    // Convert client IP from binary to readable text presentation format (inet_ntop)
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    std::cout << "[+] Client connected! Remote IP: " << client_ip 
              << ", Remote Port: " << ntohs(client_addr.sin_port)
              << ", Client FD: " << client_fd << "\n";

    // 6. Receive data from the client
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, BUFFER_SIZE);
    
    // recv() BLOCKS until data arrives from the client socket
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        std::cout << "[+] Received " << bytes_received << " bytes from client:\n";
        std::cout << "-------------------------------------------------\n";
        std::cout << buffer;
        std::cout << "-------------------------------------------------\n";

        // 7. Send a response to the client
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 26\r\n\r\nHello from C++ Network Gateway!\n";
        ssize_t bytes_sent = send(client_fd, response.c_str(), response.length(), 0);
        if (bytes_sent > 0) {
            std::cout << "[+] Sent " << bytes_sent << " byte response to client.\n";
        }
    } else if (bytes_received == 0) {
        std::cout << "[-] Client disconnected gracefully (TCP FIN received).\n";
    } else {
        perror("[-] recv failed");
    }

    // 8. Close client and server sockets
    std::cout << "[+] Closing client connection (fd: " << client_fd << ")...\n";
    close(client_fd);
    std::cout << "[+] Closing listening server socket (fd: " << server_fd << ")...\n";
    close(server_fd);

    std::cout << "=================================================\n";
    std::cout << " PHASE 1 COMPLETE!\n";
    std::cout << "=================================================\n";
    return 0;
}
