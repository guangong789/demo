#include "utility/connct.h"

using namespace gaozu::logger;

Connct::Connct(int fd) : m_fd(fd) {
    log_info("Connection created: %d", m_fd);
}

Connct::~Connct() {
    if (m_fd >= 0) {
        close(m_fd);
        log_info("Connection closed: %d", m_fd);
        m_fd = -1;
    }
}

int Connct::get_fd() const {
    return m_fd;
}

// 接收数据到独立缓冲区，不再自动发送
bool Connct::on_read(std::string& data) {
    char buf[4096];
    ssize_t len = recv(m_fd, buf, sizeof(buf), 0);
    if (len < 0) {
        if (errno == EINTR) return true;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
        log_error("recv error from client %d: errno=%d, errmsg=%s", m_fd, errno, strerror(errno));
        return false;
    } else if (len == 0) {
        log_info("Client %d disconnected", m_fd);
        return false;
    } else {
        data.assign(buf, len);
        log_info("Received from client %d: %.100s", m_fd, buf);
        return true;
    }
}

// 发送缓冲区只存待发送的数据
bool Connct::send_data(const std::string& data) {
    if (data.empty()) return true;
    m_write_data += data;

    ssize_t sent = send(m_fd, m_write_data.data(), m_write_data.size(), 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
        log_error("send error to client %d: %s", m_fd, strerror(errno));
        return false;
    }
    m_write_data.erase(0, sent);
    return true;
}

bool Connct::on_write() {
    if (m_write_data.empty()) return true;
    ssize_t sent = send(m_fd, m_write_data.data(), m_write_data.size(), 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
        log_error("on_write failed for client %d: %s", m_fd, strerror(errno));
        return false;
    }
    m_write_data.erase(0, sent);
    return true;
}

bool Connct::has_pending_data() const {
    return !m_write_data.empty();
}
