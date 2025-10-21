#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <queue>
#include <mutex>
#include <cstring>
#include <errno.h>
#include "utility/log_init.h"

using namespace gaozu::logger;

class Epoll {
public:
    Epoll();
    ~Epoll();

    bool ep_add(int fd, uint32_t event);
    bool ep_del(int fd);
    bool ep_mod(int fd, uint32_t event);
    int ep_wait(std::vector<struct epoll_event>& events, int timeout = -1);
    void request_mod(int fd, uint32_t event);
    void request_del(int fd);
    void flush_pending_ops();
private:
    int m_epfd;
    struct PendingOp {
        int fd;
        uint32_t event;
        int op; // 1=MOD, 2=DEL
        PendingOp() = default;
        PendingOp(int _fd, uint32_t _event, int _op) 
            : fd(_fd), event(_event), op(_op) {}
    };
    std::mutex pending_mtx;
    std::queue<PendingOp> pending_ops;
};