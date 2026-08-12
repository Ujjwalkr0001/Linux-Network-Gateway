#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <iostream>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>
#include "network/Connection.hpp"

class ConnectionManager {
public:
    ConnectionManager() = default;

    std::shared_ptr<Connection> add_connection(int fd, const std::string& ip, uint16_t port) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto conn = std::make_shared<Connection>(fd, ip, port);
        connections_[fd] = conn;
        return conn;
    }

    std::shared_ptr<Connection> get_connection(int fd) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(fd);
        if (it != connections_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void remove_connection(int fd) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(fd);
    }

    size_t active_connection_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connections_.size();
    }

    // Clean up idle connections that haven't sent data within timeout_sec
    std::vector<int> get_expired_connections(std::chrono::seconds timeout_sec) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<int> expired;
        for (const auto& [fd, conn] : connections_) {
            if (conn->get_idle_seconds() >= timeout_sec) {
                expired.push_back(fd);
            }
        }
        return expired;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;
};

#endif // CONNECTION_MANAGER_HPP
