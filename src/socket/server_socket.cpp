#include "socket/server_socket.h"

using namespace gaozu::socket;
using namespace gaozu::logger;

ServerSocket::ServerSocket(const std::string &ip, int port) : Socket() {
    // set_blocking();
    set_non_blocking();  // 保证服务器响应性, 不影响主线程
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

    // 设置新客户端非阻塞
    int flag = fcntl(connfd, F_GETFL, 0);
    fcntl(connfd, F_SETFL, flag | O_NONBLOCK);

    log_debug("Client %d set to non-blocking", connfd);
    return connfd;
}

std::shared_ptr<ClientSocket> ServerSocket::accept_client() {
    int connfd = ::accept(m_sockfd, nullptr, nullptr);
    if (connfd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {  // 非阻塞正常情况
            return nullptr;
        }
        log_error("accept error: errno=%d, errmsg=%s", errno, strerror(errno));
        return nullptr;
    } else {
        auto client = std::make_shared<ClientSocket>(connfd);
        client->set_blocking();
        log_info("New client connected: %d", connfd);
        return client;
    }
}