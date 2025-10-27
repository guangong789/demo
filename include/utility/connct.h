#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <mutex>
#include <atomic>
#include <memory>
#include "log_init.h"
#include "socket/client_socket.h"

using namespace gaozu::socket;

class Connct {
    public:
    explicit Connct(int fd, bool close_fd = false);
    explicit Connct(std::shared_ptr<ClientSocket> socket);
    ~Connct();

    int get_fd() const;
    bool on_read(); // 接收服务器数据
    bool send_data(const std::string& data);
    bool on_write(); // epoll 可写触发
    bool has_pending_data() const;

    void mark_closed(); // 只标记关闭
    bool is_closed() const;
    void close();
    std::string& get_read_buf();
    void clear_read_buf(size_t n);
    bool get_keep_alive() const;
    void set_keep_alive(bool val);
    bool is_epoll_registered() const;
    void mark_if_registered(bool val);

    private:
    int m_fd;
    std::atomic<bool> closed = false; // 线程安全关闭标记
    std::atomic<bool> epoll_registered = true;
    std::mutex mtx;
    std::shared_ptr<ClientSocket> client;
    std::string m_write_data; // 发送缓冲区
    std::string m_read_data;  // 接收缓冲区
    bool keep_alive = false;
};