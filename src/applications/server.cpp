#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <algorithm>
#include "socket/server_socket.h"
#include "utility/log_init.h"
#include "utility/epoll.h"
using namespace test::socket;
using namespace gaozu::logger;

int main(int argc, char* argv[]) {
    gaozu::init::LoggerInitializer::create("logs/server.log")
        .set_console(true)
        .set_level(Logger::INFO)
        .set_max_size(1 * 1024)
        .set_backup_count(1);

    // 创建服务器socket
    gaozu::socket::ServerSocket server("127.0.0.1", 8080);
    log_info("Server started on 127.0.0.1:8080 using epoll");
    // 初始化
    Epoll ep;
    ep.ep_add(server.get_fd(), EPOLLIN);
    const int MAX_EVENTS = 1024;
    std::vector<struct epoll_event> events(MAX_EVENTS);
    while (true) {
        // wait and fetch，内核操作
        int nready = ep.ep_wait(events, -1);
        // 遍历所有触发的事件
        for (int i = 0; i < nready; ++i) {
            int fd = events[i].data.fd;
            if (fd == server.get_fd()) {  // 是服务端
                while(true) {
                    int connfd = server.accept();
                    if (connfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // 没有更多连接
                        log_error("accept error: errno=%d, errmsg=%s", errno, strerror(errno));
                        break;
                    }
                    log_info("New client connected: %d", connfd);
                    ep.ep_add(connfd, EPOLLIN);
                }
            } else if (events[i].events & EPOLLIN) {  // 客户端有数据可读
                char buf[1024] = {0};
                ssize_t len = recv(fd, buf, sizeof(buf)-1, 0);
                if (len < 0) {
                    if (errno == EINTR) continue;
                    log_error("recv error from client %d: errno=%d, errmsg=%s", fd, errno, strerror(errno));
                    ep.ep_delete(fd);
                    continue;
                }
                if (len == 0) {  // 客户端关闭连接
                    log_info("Client %d disconnected", fd);
                    ep.ep_delete(fd);
                    continue;
                }
                buf[len] = '\0';
                log_info("Received from client %d: %.100s", fd, buf);
                // 回显数据
                ssize_t sent = 0;
                while (sent < len) {
                    ssize_t ret = send(fd, buf + sent, len - sent, 0);
                    if (ret < 0) {
                        if (errno == EINTR) continue;
                        log_error("Send error to client %d: errno=%d, errmsg=%s", fd, errno, strerror(errno));
                        ep.ep_delete(fd);
                        break;
                    }
                    sent += ret;
                }
            } else if (events[i].events & (EPOLLERR | EPOLLHUP)) {  // 异常事件
                log_warn("Client %d error or hang up", fd);
                ep.ep_delete(fd);
            }
        }
    }
    // 清理资源
    log_info("Shutting down server...");
    // 关闭socket
    server.close();
    return 0;
}