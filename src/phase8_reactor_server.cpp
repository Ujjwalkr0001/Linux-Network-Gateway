#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "core/EpollReactor.hpp"
#include "network/SocketUtils.hpp"

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

class GatewayReactorServer {
public:
    GatewayReactorServer(int port) : port_(port), server_fd_(-1) {}

    ~GatewayReactorServer() {
        if (server_fd_ >= 0) close(server_fd_);
    }

    bool start() {
        // 1. Initialize listening socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) return false;

        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        SocketUtils::set_non_blocking(server_fd_);

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if (listen(server_fd_, SOMAXCONN) < 0) return false;

        std::cout << "[+] Server Listening on 0.0.0.0:" << port_ << "\n";

        // 2. Register Listening Socket in Reactor for EPOLLIN (Accept Callback)
        reactor_.add_socket(server_fd_, EPOLLIN, [this](int fd, uint32_t events) {
            this->handle_accept(fd, events);
        });

        // 3. Start Reactor Event Loop
        reactor_.run();
        return true;
    }

private:
    void handle_accept(int server_fd, uint32_t events) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd >= 0) {
            SocketUtils::set_non_blocking(client_fd);

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            std::cout << "[+] [Reactor] Accept Event: Client " << client_ip 
                      << ":" << ntohs(client_addr.sin_port) << " (FD: " << client_fd << ")\n";

            // Register Client Socket in Reactor for EPOLLIN (I/O Callback)
            reactor_.add_socket(client_fd, EPOLLIN | EPOLLHUP | EPOLLERR, 
                [this](int fd, uint32_t events) {
                    this->handle_client_io(fd, events);
                });
        }
    }

    void handle_client_io(int client_fd, uint32_t events) {
        if (events & (EPOLLHUP | EPOLLERR)) {
            std::cout << "[-] [Reactor] Disconnect/Error Event on FD " << client_fd << "\n";
            reactor_.remove_socket(client_fd);
            close(client_fd);
            return;
        }

        if (events & EPOLLIN) {
            char buffer[BUFFER_SIZE];
            std::memset(buffer, 0, BUFFER_SIZE);

            ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes > 0) {
                std::cout << "[+] [Reactor Data Event FD " << client_fd << "] Recv " 
                          << bytes << " bytes: " << buffer;
                // Echo back
                send(client_fd, buffer, bytes, 0);
            } else if (bytes == 0) {
                std::cout << "[-] [Reactor] Client FD " << client_fd << " closed connection.\n";
                reactor_.remove_socket(client_fd);
                close(client_fd);
            } else {
                if (!SocketUtils::is_would_block()) {
                    reactor_.remove_socket(client_fd);
                    close(client_fd);
                }
            }
        }
    }

    int port_;
    int server_fd_;
    EpollReactor reactor_;
};

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 8: SCALABLE EVENT-DRIVEN REACTOR CORE\n";
    std::cout << "=================================================\n";

    GatewayReactorServer server(PORT);
    server.start();

    return 0;
}
