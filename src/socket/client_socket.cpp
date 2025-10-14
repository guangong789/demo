#include "socket/client_socket.h"
using namespace gaozu::socket;
using namespace gaozu::logger;

ClientSocket::ClientSocket(int fd) : Socket() {
    m_sockfd = fd;
    m_non_blocking = (fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != 0;
}

ClientSocket::ClientSocket(const std::string &ip, int port, bool non_blocking) : Socket(), m_non_blocking(non_blocking) {
    if (m_non_blocking) {
        set_non_blocking(true);
    }
    connect(ip, port);
}

ClientSocket::~ClientSocket() {}

void ClientSocket::close_fd() {
    std::unique_lock<std::mutex> locker(mtx);
    if (m_sockfd >= 0) {
        ::close(m_sockfd);
        m_sockfd = -1;
    }
}

bool ClientSocket::set_non_blocking(bool non_blocking) {
    if (m_sockfd < 0) {
        log_error("set_non_blocking: invalid socket fd");
        return false;
    }
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    if (flags == -1) {
        log_error("fcntl(F_GETFL) failed: %s", strerror(errno));
        return false;
    }
    if (fcntl(m_sockfd, F_SETFL, non_blocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == -1) {
        log_error("fcntl(F_SETFL) failed: %s", strerror(errno));
        return false;
    }

    m_non_blocking = non_blocking;
    return true;
}