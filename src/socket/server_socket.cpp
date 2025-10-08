#include "socket/server_socket.h"

using namespace gaozu::socket;
using namespace gaozu::logger;

ServerSocket::ServerSocket(const std::string &ip, int port) : Socket() {
    // set_blocking();
    set_non_blocking();
    set_recv_buffer(10 * 1024);
    set_send_buffer(10 * 1024);
    set_linger(true, 0);
    set_keepalive();
    set_reuseaddr();
    bind(ip, port);
    listen(1024);
}

ServerSocket::~ServerSocket() {}

int ServerSocket::accept() {
    int connfd = ::accept(m_sockfd, nullptr, nullptr);
    if (connfd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;  // 正常情况
        log_error("socket accept error: errno = %d, errmsg = %s", errno, strerror(errno));
        return -1;
    }

    // 新客户端设置为阻塞
    int flags = fcntl(connfd, F_GETFL, 0);
    fcntl(connfd, F_SETFL, flags & ~O_NONBLOCK);

    return connfd;
}
