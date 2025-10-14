#pragma once
#include <unordered_map>
#include <shared_mutex>
#include "connct.h"

class ConnManager {
public:
    ConnManager() = default;
    ~ConnManager() = default;

    void add(int fd, std::shared_ptr<Connct> conn);
    void remove(int fd);
    std::shared_ptr<Connct> get_ptr(int fd);
    void for_each(std::function<void(int, std::shared_ptr<Connct>&)> fn);
    void close_all();

private:
    std::unordered_map<int, std::shared_ptr<Connct>> m_conns;
    std::shared_mutex shared_mtx;
};