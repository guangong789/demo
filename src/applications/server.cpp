#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <csignal>
#include "socket/server_socket.h"
#include "utility/connct.h"
#include "utility/log_init.h"
#include "utility/epoll.h"
#include "utility/threadpool.h"
#include "utility/conn_manager.h"
#include "http/request.h"
#include "http/response.h"
#include "http/router.h"

using namespace test::socket;
using namespace gaozu::logger;

std::atomic<bool> running = true;

void handle_sigint(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, handle_sigint);

    LoggerInitializer::create("logs/server.log")
        .set_console(true)
        .set_level(Logger::INFO)
        .set_max_size(10 * 1024);
    // 初始化router
    Router::instance().add_route("/", [](const Request&) {
        return Response::make_html(200, "<h1>Hello, HTTP Server!</h1>");
    });
    Router::instance().add_route("/api/echo", [](const Request& req) {
        return Response::make_json(200, "{\"echo\":\"" + req.body + "\"}");
    });
    // 创建服务器socket
    gaozu::socket::ServerSocket server("127.0.0.1", 8080);
    log_info("Server started on 127.0.0.1:8080 using epoll");
    // 初始化epoll
    Epoll ep;
    ep.ep_add(server.get_fd(), EPOLLIN | EPOLLET);
    const int MAX_EVENTS = 1024;
    std::vector<struct epoll_event> events(MAX_EVENTS);
    ConnManager conn_mng;
    Threadpool pool;

    while (running) {
        // wait and fetch，内核操作
        int nready = ep.ep_wait(events, 500);
        if (!running) break;
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
                    auto conn = std::make_shared<Connct>(client);
                    int connfd = conn->get_fd();
                    ep.ep_add(connfd, EPOLLIN | EPOLLET);
                    conn_mng.add(connfd, conn);
                }
            } else if (ev & (EPOLLIN | EPOLLOUT)) {
                auto conn = conn_mng.get_ptr(fd);
                if (!conn) continue;
                auto ep_ptr = &ep;
                auto conn_mng_ptr = &conn_mng;
                pool.add_task([fd, conn, ev, ep_ptr, conn_mng_ptr]() {
                    if (ev & EPOLLIN) {
                        bool ok = conn->on_read();
                        if (!ok) {
                            ep_ptr->ep_del(fd);
                            conn->close();
                            conn_mng_ptr->remove(fd);
                            return;
                        }
                        while (Request::is_complete(conn->get_read_buf())) {
                            size_t req_len = Request::get_total_length(conn->get_read_buf());
                            Request req = Request::parse(conn->get_read_buf().substr(0, req_len));

                            conn->set_keep_alive(req.keep_alive);
                            Response resp = Router::instance().handler(req);
                            if (!conn->send_data(resp.serialize(req.keep_alive))) {
                                ep_ptr->ep_del(fd);
                                conn->close();
                                conn_mng_ptr->remove(fd);
                                return;
                            }
                            conn->clear_read_buf(req_len);
                            if (!conn->get_keep_alive()) {
                                ep_ptr->ep_del(fd);
                                conn->close();
                                conn_mng_ptr->remove(fd);
                                return;
                            }
                        }
                        uint32_t new_event = EPOLLIN;
                        if (conn->has_pending_data()) new_event |= EPOLLOUT;
                        ep_ptr->ep_mod(fd, new_event);
                    }
                    if (ev & EPOLLOUT) {
                        bool ok = conn->on_write();
                        if (!ok) {
                            ep_ptr->ep_del(fd);
                            conn->close();
                            conn_mng_ptr->remove(fd);
                        } else if (!conn->has_pending_data()) {
                            ep_ptr->ep_mod(fd, EPOLLIN);
                        }
                    }
                });
            } else if (ev & (EPOLLERR | EPOLLHUP)) {  // 异常事件
                log_warn("Client %d error or hang up", fd);
                ep.ep_del(fd);
                conn_mng.remove(fd);
            }
        }
    }

    log_info("Shutting down server...");
    conn_mng.close_all();
    server.close();
    return 0;
}