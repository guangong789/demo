#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <mutex>
#include <atomic>
#include "log_init.h"
#include "socket/client_socket.h"
using namespace gaozu::socket;

class Connct {
public:
    explicit Connct(int fd, bool close = false);
    explicit Connct(std::shared_ptr<ClientSocket> socket);
    ~Connct();

    int get_fd() const;
    bool on_read(std::string& data);  // 接收服务器数据和触发回应
    bool send_data(const std::string& data);  // 主动触发的写入
    bool on_write();  // epoll调用：内核缓冲区可以写入了
    bool has_pending_data() const;  // 发送缓冲区有无数据
    void close();
    bool is_closed() const;
    void change_closed();

private:
    int m_fd;
    std::atomic<bool> closed = false;
    std::mutex mtx;
    std::shared_ptr<ClientSocket> client;
    std::string m_write_data;  // 发送缓冲区
};