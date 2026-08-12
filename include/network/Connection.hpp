#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <memory>

enum class ConnectionState {
    CONNECTED,
    AUTHENTICATING,
    AUTHENTICATED,
    CLOSING,
    CLOSED
};

inline std::string state_to_string(ConnectionState state) {
    switch (state) {
        case ConnectionState::CONNECTED:      return "CONNECTED";
        case ConnectionState::AUTHENTICATING: return "AUTHENTICATING";
        case ConnectionState::AUTHENTICATED:  return "AUTHENTICATED";
        case ConnectionState::CLOSING:        return "CLOSING";
        case ConnectionState::CLOSED:         return "CLOSED";
        default:                              return "UNKNOWN";
    }
}

class Connection {
public:
    Connection(int fd, const std::string& client_ip, uint16_t client_port)
        : fd_(fd), client_ip_(client_ip), client_port_(client_port),
          state_(ConnectionState::CONNECTED),
          bytes_received_(0), bytes_sent_(0), packets_processed_(0) {
        last_activity_ = std::chrono::steady_clock::now();
    }

    int get_fd() const { return fd_; }
    std::string get_ip() const { return client_ip_; }
    uint16_t get_port() const { return client_port_; }

    ConnectionState get_state() const { return state_; }
    void set_state(ConnectionState new_state) {
        std::cout << "[+] [State Transition FD " << fd_ << "] " 
                  << state_to_string(state_) << " -> " << state_to_string(new_state) << "\n";
        state_ = new_state;
        touch();
    }

    void touch() {
        last_activity_ = std::chrono::steady_clock::now();
    }

    std::chrono::seconds get_idle_seconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_);
    }

    void record_recv(size_t bytes) {
        bytes_received_ += bytes;
        packets_processed_++;
        touch();
    }

    void record_send(size_t bytes) {
        bytes_sent_ += bytes;
        touch();
    }

    uint64_t get_bytes_received() const { return bytes_received_; }
    uint64_t get_bytes_sent() const { return bytes_sent_; }
    uint64_t get_packets_processed() const { return packets_processed_; }

    // Read/Write Buffer management for partial TCP framing
    std::vector<uint8_t> rx_buffer;
    std::vector<uint8_t> tx_buffer;

private:
    int fd_;
    std::string client_ip_;
    uint16_t client_port_;
    ConnectionState state_;
    std::chrono::steady_clock::time_point last_activity_;

    uint64_t bytes_received_;
    uint64_t bytes_sent_;
    uint64_t packets_processed_;
};

#endif // CONNECTION_HPP
