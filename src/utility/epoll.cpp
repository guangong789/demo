#include "utility/epoll.h"

Epoll::Epoll() : m_epfd(-1), conn_mng(nullptr) {
    m_epfd = epoll_create1(0);
    if (m_epfd < 0) {
        log_error("epoll_create1 failed: errno=%d errmsg=%s", errno, strerror(errno));
    } else {
        log_debug("Epoll created successfully, epfd=%d", m_epfd);
    }
}

Epoll::~Epoll() {
    if (m_epfd >= 0) {
        close(m_epfd);
        m_epfd = -1;
        log_debug("Epoll destroyed, epfd closed");
    }
}

bool Epoll::ep_add(int fd, uint32_t event) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        log_error("epoll_ctl ADD client failed: errno=%d, errmsg=%s", errno, strerror(errno));
        close(fd);
        return false;
    }
    return true;
}

bool Epoll::ep_del(int fd) {
    if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        log_error("epoll_ctl DEL failed: fd=%d errno=%d errmsg=%s", fd, errno, strerror(errno));
        return false;
    }
    return true;
}

bool Epoll::ep_mod(int fd, uint32_t event) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if (epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        log_error("epoll_ctl MOD failed: fd=%d errno=%d errmsg=%s", fd, errno, strerror(errno));
        return false;
    }
    return true;
}

int Epoll::ep_wait(std::vector<struct epoll_event>& events, int timeout) {
    int nready = epoll_wait(m_epfd, events.data(), (int)events.size(), timeout);
    if (nready < 0 && errno != EINTR) {
        log_error("epoll_wait failed: errno=%d errmsg=%s", errno, strerror(errno));
    }
    return nready;
}

void Epoll::request_mod(int fd, uint32_t event) {
    std::lock_guard<std::mutex> locker(pending_mtx);
    pending_ops.emplace(fd, event, 1);
}

void Epoll::request_del(int fd) {
    std::lock_guard<std::mutex> locker(pending_mtx);
    pending_ops.emplace(fd, 0, 2);
}

void Epoll::flush_pending_ops() {
    if (!conn_mng) return;
    std::lock_guard<std::mutex> lk(pending_mtx);

    while (!pending_ops.empty()) {
        auto op = pending_ops.front();
        pending_ops.pop();
        if (op.op == 1) { // MOD
            ep_mod(op.fd, op.event);
        } else if (op.op == 2) { // DEL
            auto conn = conn_mng->get_ptr(op.fd);
            if (!conn || !conn->is_epoll_registered() || conn->is_closed()) continue;
            if (ep_del(op.fd)) conn->mark_if_registered(false);
        }
    }
}
