#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <functional>
#include "connct.h"

class Epoll;

class ConnManager {
public:
    ConnManager() = default;
    ~ConnManager() = default;

    void add(int fd, std::shared_ptr<Connct> conn);
    void remove(int fd);
    std::shared_ptr<Connct> get_ptr(int fd);
    void for_each(std::function<void(int, std::shared_ptr<Connct>&)> fn);
    void sweep_closed(Epoll& ep);
    void close_all(Epoll* ep = nullptr);
    void sweep_inactive(Epoll& ep, int timeout_sec);
    void touch(int fd);  // 更新连接最后活跃时间

private:
    struct ConnInfo {
        std::shared_ptr<Connct> conn;
        std::chrono::steady_clock::time_point last_active;  // 最后活跃时间
    };

    std::unordered_map<int, ConnInfo> m_conns;
    std::shared_mutex shared_mtx;
};