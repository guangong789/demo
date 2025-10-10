#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include <string>
#include "log_init.h"

class Connct {
public:
    explicit Connct(int fd);

    ~Connct();

    int get_fd() const;
    bool on_read(std::string& data);  // 接收和触发回应
    bool send_data(const std::string& data);  // 请求发送数据，主动调用
    bool on_write();  // epoll调用：可以写入了
    bool has_pending_data() const;  // 缓冲区有无数据

private:
    int m_fd;
    std::string m_write_data;
};