#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <algorithm>
#include <unordered_map>
#include "socket/server_socket.h"
#include "utility/connct.h"
#include "utility/log_init.h"
#include "utility/epoll.h"

using namespace test::socket;
using namespace gaozu::logger;

int main(int argc, char* argv[]) {
    LoggerInitializer::create("logs/server.log")
        .set_console(true)
        .set_level(Logger::INFO)
        .set_max_size(10 * 1024)
        .set_backup_count(1);

    // 创建服务器socket
    gaozu::socket::ServerSocket server("127.0.0.1", 8080);
    log_info("Server started on 127.0.0.1:8080 using epoll");
    // 初始化
    Epoll ep;
    ep.ep_add(server.get_fd(), EPOLLIN);
    const int MAX_EVENTS = 1024;
    std::vector<struct epoll_event> events(MAX_EVENTS);
    std::unordered_map<int, std::shared_ptr<Connct>> connections;

    while (true) {
        // wait and fetch，内核操作
        int nready = ep.ep_wait(events, -1);
        // 遍历所有触发的事件
        for (int i = 0; i < nready; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == server.get_fd()) {  // 是服务端
                while (true) {
                    auto client = server.accept_client();
                    if (!client) {
                        break;
                    }
                    int connfd = client->release_fd();  // 防止double close
                    ep.ep_add(connfd, EPOLLIN);
                    auto conn = std::make_shared<Connct>(connfd);
                    connections[connfd] = conn;
                }
            } else if (ev & EPOLLIN) {  // 客户端有数据可读
                auto it = connections.find(fd);
                if (it == connections.end()) continue;

                auto conn = it->second;
                std::string received;
                bool ok = conn->on_read(received);
                if (!ok) {
                    ep.ep_del(fd);
                    connections.erase(it);
                    shutdown(fd, SHUT_RDWR);
                } else {
                    if (!received.empty()) {
                        conn->send_data(received);
                    }
                    if (conn->has_pending_data()) {
                        ep.ep_mod(fd, EPOLLIN | EPOLLOUT);
                    } else {
                        ep.ep_mod(fd, EPOLLIN);
                    }
                }
            } else if (ev & EPOLLOUT) {  // 监视可写
                auto it = connections.find(fd);
                if (it == connections.end()) continue;

                auto conn = it->second;
                bool ok = conn->on_write();
                if (!ok) {
                    ep.ep_del(fd);
                    connections.erase(it);
                    shutdown(fd, SHUT_RDWR);
                }
            } else if (ev & (EPOLLERR | EPOLLHUP)) {  // 异常事件
                log_warn("Client %d error or hang up", fd);
                ep.ep_del(fd);
                connections.erase(fd);
                shutdown(fd, SHUT_RDWR);
            }
        }
    }
    
    log_info("Shutting down server...");
    server.close();
    return 0;
}