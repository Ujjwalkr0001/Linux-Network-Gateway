#ifndef RAII_SOCKET_HPP
#define RAII_SOCKET_HPP

#include <iostream>
#include <unistd.h>
#include <utility>
#include <cerrno>

// ─────────────────────────────────────────────────────────────────────────
// RAII SOCKET WRAPPER
//
// Guarantees exception-safe lifetime management for Linux File Descriptors.
// Ensures close(fd) is invoked upon scope exit, preventing descriptor leaks.
// Enforces Move-Only semantics (Non-Copyable) to prevent double-closing.
// ─────────────────────────────────────────────────────────────────────────

class RAIISocket {
public:
    // Construct with optional raw file descriptor
    explicit RAIISocket(int fd = -1) : fd_(fd) {}

    // Destructor: Automatic cleanup of OS socket resource
    ~RAIISocket() {
        close_fd();
    }

    // Disable Copying (Prevents double close bug)
    RAIISocket(const RAIISocket&) = delete;
    RAIISocket& operator=(const RAIISocket&) = delete;

    // Enable Move Semantics (Transfer of Ownership)
    RAIISocket(RAIISocket&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    RAIISocket& operator=(RAIISocket&& other) noexcept {
        if (this != &other) {
            close_fd();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    // Access underlying file descriptor
    int get() const { return fd_; }
    bool is_valid() const { return fd_ >= 0; }

    // Release ownership of raw file descriptor without closing it
    int release() {
        int temp = fd_;
        fd_ = -1;
        return temp;
    }

    // Replace current socket with a new descriptor
    void reset(int new_fd = -1) {
        close_fd();
        fd_ = new_fd;
    }

private:
    void close_fd() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    int fd_;
};

#endif // RAII_SOCKET_HPP
