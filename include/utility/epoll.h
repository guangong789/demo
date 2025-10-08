#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <errno.h>
#include "utility/log_init.h"

using namespace gaozu::logger;

class Epoll {
public:
    Epoll();
    ~Epoll();

    bool ep_add(int fd, uint32_t event);
    bool ep_delete(int fd);
    bool ep_modify(int fd, uint32_t event);
    int ep_wait(std::vector<struct epoll_event>& events, int timeout = -1);
private:
    int m_epfd;
};