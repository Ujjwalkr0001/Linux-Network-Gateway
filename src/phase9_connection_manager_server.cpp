#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "core/EpollReactor.hpp"
#include "network/ConnectionManager.hpp"
#include "network/SocketUtils.hpp"

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 9: STATE-MACHINE CONNECTION MANAGER\n";
    std::cout << "=================================================\n";

    ConnectionManager conn_mgr;

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

    EpollReactor reactor;

    // Register listening socket callback
    reactor.add_socket(server_fd, EPOLLIN, [&](int s_fd, uint32_t events) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(s_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd >= 0) {
            SocketUtils::set_non_blocking(client_fd);
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            uint16_t client_port = ntohs(client_addr.sin_port);

            // Add connection to ConnectionManager (State: CONNECTED)
            auto conn = conn_mgr.add_connection(client_fd, std::string(client_ip), client_port);
            std::cout << "[+] New Connection Managed (FD: " << client_fd 
                      << ", Active Count: " << conn_mgr.active_connection_count() << ")\n";

            // Move state to AUTHENTICATING
            conn->set_state(ConnectionState::AUTHENTICATING);

            // Register client in EpollReactor
            reactor.add_socket(client_fd, EPOLLIN | EPOLLHUP | EPOLLERR, [&](int c_fd, uint32_t c_events) {
                auto active_conn = conn_mgr.get_connection(c_fd);
                if (!active_conn) return;

                if (c_events & (EPOLLHUP | EPOLLERR)) {
                    active_conn->set_state(ConnectionState::CLOSING);
                    reactor.remove_socket(c_fd);
                    conn_mgr.remove_connection(c_fd);
                    close(c_fd);
                    std::cout << "[-] Connection Closed (Active Count: " << conn_mgr.active_connection_count() << ")\n";
                    return;
                }

                if (c_events & EPOLLIN) {
                    char buffer[BUFFER_SIZE];
                    std::memset(buffer, 0, BUFFER_SIZE);

                    ssize_t bytes = recv(c_fd, buffer, BUFFER_SIZE - 1, 0);
                    if (bytes > 0) {
                        active_conn->record_recv(bytes);

                        std::cout << "[+] [FD " << c_fd << "] [State: " 
                                  << state_to_string(active_conn->get_state()) << "] Recv " 
                                  << bytes << " bytes (Total: " << active_conn->get_bytes_received() << " B)\n";

                        // Transition to AUTHENTICATED on first payload
                        if (active_conn->get_state() == ConnectionState::AUTHENTICATING) {
                            active_conn->set_state(ConnectionState::AUTHENTICATED);
                        }

                        // Echo back
                        send(c_fd, buffer, bytes, 0);
                        active_conn->record_send(bytes);

                    } else if (bytes == 0) {
                        active_conn->set_state(ConnectionState::CLOSING);
                        reactor.remove_socket(c_fd);
                        conn_mgr.remove_connection(c_fd);
                        close(c_fd);
                        std::cout << "[-] Client Disconnected cleanly (Remaining Active: " 
                                  << conn_mgr.active_connection_count() << ")\n";
                    }
                }
            });
        }
    });

    std::cout << "[+] State-Machine Connection Gateway listening on port " << PORT << "...\n";
    reactor.run();

    close(server_fd);
    return 0;
}
