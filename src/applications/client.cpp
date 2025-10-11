#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>
#include "utility/epoll.h"
#include "utility/connct.h"
#include "socket/client_socket.h"
#include "utility/log_init.h"

using namespace test::socket;
using namespace gaozu::logger;

int main(int argc, char* argv[]) {
    LoggerInitializer::create("logs/client.log")
        .set_console(true)
        .set_level(Logger::INFO)
        .set_max_size(10 * 1024)
        .set_backup_count(1);

    gaozu::socket::ClientSocket client("127.0.0.1", 8080);
    log_info("Connected to server 127.0.0.1:8080 using epoll");

    Epoll ep;
    int sockfd = client.get_fd();
    ep.ep_add(sockfd, EPOLLIN);
    ep.ep_add(STDIN_FILENO, EPOLLIN);
    const int MAX_SIZE = 64;
    std::vector<struct epoll_event> events(MAX_SIZE);
    auto conn = std::make_shared<Connct>(sockfd);

    std::atomic<bool> running(true);  // 多线程内部可见
    std::string input_buffer;  // 用户命令

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    while (running) {
        int nready = ep.ep_wait(events, -1);

        for (int i = 0; i < nready; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == sockfd && (ev & EPOLLIN)) {  // 服务器数据到达
                std::string received;
                bool ok = conn->on_read(received);
                if (!ok) {
                    log_info("Server closed connection");
                    running = false;
                    break;
                }
                if (!received.empty()) {
                    std::cout << "[Server] " << received << std::endl;
                }
            } else if (fd == STDIN_FILENO && (ev & EPOLLIN)) {  // 用户输入处理
                char buf[1024];
                ssize_t len = read(STDIN_FILENO, buf, sizeof(buf));
                if (len > 0) {
                    input_buffer.append(buf, len);
                    size_t pos;
                    while ((pos = input_buffer.find('\n')) != std::string::npos) {  // 凑齐一行再发出
                        std::string line = input_buffer.substr(0, pos);
                        input_buffer.erase(0, pos + 1);

                        if (line == "exit") {
                            running = false;
                            break;
                        }
                        if (!line.empty()) {
                            conn->send_data(line);
                            if (conn->has_pending_data()) {
                                ep.ep_mod(sockfd, EPOLLIN | EPOLLOUT);
                                // 监听一下什么时候可以写入内核缓冲区
                            }
                        }
                    }
                }
            } else if (fd == sockfd && (ev & EPOLLOUT)) {
                bool ok = conn->on_write();
                if (!ok) {
                    running = false;
                    break;
                }
                if (conn->has_pending_data()) {
                    ep.ep_mod(sockfd, EPOLLIN | EPOLLOUT);
                } else {  //  没有数据，回到只读模式
                    ep.ep_mod(sockfd, EPOLLIN);
                }
            } else if (ev & (EPOLLHUP | EPOLLERR)) {  // 异常处理
                log_warn("connection error/hangup");
                running = false;
                break;
            }
        }
    }

    client.close();
    log_info("Client exit.");
    return 0;
}
