#include <mutex>
#include <unistd.h>
#include "utility/conn_manager.h"

void ConnManager::add(int fd, std::shared_ptr<Connct> conn) {
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    m_conns[fd] = conn;
}

void ConnManager::remove(int fd) {
    std::shared_ptr<Connct> conn;
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    auto it = m_conns.find(fd);
    if (it != m_conns.end()) {
        conn = it->second;
        m_conns.erase(it);
    }
}

std::shared_ptr<Connct> ConnManager::get_ptr(int fd) {
    std::shared_lock<std::shared_mutex> locker(shared_mtx);
    auto it = m_conns.find(fd);
    if (it != m_conns.end()) {
        return it->second;
    }
    return nullptr;
}

void ConnManager::for_each(std::function<void(int, std::shared_ptr<Connct>&)> f) {
    std::shared_lock<std::shared_mutex> locker(shared_mtx);
    for (auto& [fd, conn] : m_conns) {
        f(fd, conn);
    }
}

void ConnManager::close_all() {
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    for (auto& [fd, conn] : m_conns) {
        if (conn) {
            conn->close();
        } else {
            ::close(fd);
        }
    }
    m_conns.clear();
}