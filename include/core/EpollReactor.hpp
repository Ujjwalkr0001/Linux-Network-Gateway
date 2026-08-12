#ifndef EPOLL_REACTOR_HPP
#define EPOLL_REACTOR_HPP

#include <iostream>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <atomic>

class EpollReactor {
public:
    using EventCallback = std::function<void(int fd, uint32_t events)>;

    explicit EpollReactor(int max_events = 64) 
        : max_events_(max_events), running_(false) {
        
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ < 0) {
            throw std::runtime_error("Failed to create epoll instance");
        }
        events_.resize(max_events_);
    }

    ~EpollReactor() {
        if (epoll_fd_ >= 0) {
            close(epoll_fd_);
        }
    }

    // Register a file descriptor into epoll with an associated callback
    bool add_socket(int fd, uint32_t event_flags, EventCallback callback) {
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = event_flags;
        ev.data.fd = fd;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            perror("epoll_ctl EPOLL_CTL_ADD failed");
            return false;
        }

        callbacks_[fd] = callback;
        return true;
    }

    // Modify events for an existing socket descriptor
    bool modify_socket(int fd, uint32_t event_flags) {
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.events = event_flags;
        ev.data.fd = fd;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
            perror("epoll_ctl EPOLL_CTL_MOD failed");
            return false;
        }
        return true;
    }

    // Remove a socket descriptor from epoll and unregister its callback
    bool remove_socket(int fd) {
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
            perror("epoll_ctl EPOLL_CTL_DEL failed");
        }
        callbacks_.erase(fd);
        return true;
    }

    // Main Reactor Event Dispatching Loop
    void run() {
        running_ = true;
        std::cout << "[+] EpollReactor event loop started.\n";

        while (running_) {
            int nfds = epoll_wait(epoll_fd_, events_.data(), max_events_, 500); // 500ms timeout
            if (nfds < 0) {
                if (errno == EINTR) continue; // Interrupted by signal, resume
                perror("epoll_wait failed");
                break;
            }

            for (int i = 0; i < nfds; ++i) {
                int fd = events_[i].data.fd;
                uint32_t revents = events_[i].events;

                auto it = callbacks_.find(fd);
                if (it != callbacks_.end()) {
                    // Dispatch event callback!
                    it->second(fd, revents);
                }
            }
        }

        std::cout << "[+] EpollReactor event loop stopped.\n";
    }

    void stop() {
        running_ = false;
    }

    bool is_running() const { return running_; }

private:
    int epoll_fd_;
    int max_events_;
    std::atomic<bool> running_;
    std::vector<struct epoll_event> events_;
    std::unordered_map<int, EventCallback> callbacks_;
};

#endif // EPOLL_REACTOR_HPP
