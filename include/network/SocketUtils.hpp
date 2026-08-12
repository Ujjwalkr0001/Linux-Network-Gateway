#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

namespace SocketUtils {

// Sets a socket File Descriptor to non-blocking I/O mode using Linux fcntl()
inline bool set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK failed");
        return false;
    }

    return true;
}

// Checks if system call error was caused by resource temporarily unavailable (would block)
inline bool is_would_block() {
    return (errno == EAGAIN || errno == EWOULDBLOCK);
}

} // namespace SocketUtils

#endif // SOCKET_UTILS_HPP
