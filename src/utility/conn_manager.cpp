#include <mutex>
#include <unistd.h>
#include "utility/conn_manager.h"
#include "utility/epoll.h"

void ConnManager::add(int fd, std::shared_ptr<Connct> conn) {
    std::unique_lock locker(shared_mtx);
    m_conns[fd] = ConnInfo{conn, std::chrono::steady_clock::now()};
}

void ConnManager::remove(int fd) {
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    auto it = m_conns.find(fd);
    if (it != m_conns.end()) {
        m_conns.erase(it);
    }
}

std::shared_ptr<Connct> ConnManager::get_ptr(int fd) {
    std::shared_lock<std::shared_mutex> locker(shared_mtx);
    auto it = m_conns.find(fd);
    if (it != m_conns.end()) {
        return (it->second).conn;
    }
    return nullptr;
}

void ConnManager::for_each(std::function<void(int, std::shared_ptr<Connct>&)> f) {
    std::shared_lock<std::shared_mutex> locker(shared_mtx);
    for (auto& [fd, conninfo] : m_conns) {
        f(fd, conninfo.conn);
    }
}

void ConnManager::sweep_closed(Epoll& ep) {
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    std::vector<int> to_remove;
    for (auto& [fd, conninfo] : m_conns) {
        auto conn = conninfo.conn;
        if (conn && conn->is_closed()) {
            ep.ep_del(fd);
            conn->close();
            to_remove.emplace_back(fd);
        }
    }
    for (int fd : to_remove) {
        m_conns.erase(fd);
    }
}

void ConnManager::close_all(Epoll* ep) {
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    for (auto& [fd, conninfo] : m_conns) {
        if (ep) ep->ep_del(fd);
        if (conninfo.conn) (conninfo.conn)->close();
        else ::close(fd);
    }
    m_conns.clear();
}

void ConnManager::sweep_inactive(Epoll& ep, int timeout_sec) {
    auto now = std::chrono::steady_clock::now();
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    std::vector<int> to_remove;

    for (auto& [fd, conninfo] : m_conns) {
        if (!conninfo.conn) continue;
        auto idle = std::chrono::duration_cast<std::chrono::seconds>(now - conninfo.last_active).count();
        if (idle > timeout_sec) {
            log_info("Connection %d idle for %llds, closing", fd, (long long)idle);
            ep.request_del(fd);
            conninfo.conn->mark_closed();
        }
    }
}

void ConnManager::touch(int fd) {
    std::unique_lock<std::shared_mutex> locker(shared_mtx);
    auto it = m_conns.find(fd);
    if (it != m_conns.end()) {
        (it->second).last_active = std::chrono::steady_clock::now();
    }
}