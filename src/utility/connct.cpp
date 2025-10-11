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

bool Connct::on_read(std::string& data) {
    char buf[1024];
    ssize_t len = recv(m_fd, buf, sizeof(buf), 0);  // 从内核缓冲区读数据到buf
    if (len < 0) {
        if (errno == EINTR) return true;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return true;  // 没有数据可读
        log_error("recv error from client %d: errno=%d, errmsg=%s", m_fd, errno, strerror(errno));
        return false;
    } else if (len == 0) {
        log_info("Client %d disconnected", m_fd);
        return false;
    } else {
        data.assign(buf, len);  // assign能够处理'\0'，存到外部data中
        log_info("Received from client %d: %.100s", m_fd, buf);
        return true;
    }
}

bool Connct::send_data(const std::string& data) {
    if (data.empty()) return true;
    m_write_data += data;
    // 先存到发送缓冲区，再尝试立即发送一次
    ssize_t sent = send(m_fd, m_write_data.data(), m_write_data.size(), 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return true;  // 内核缓冲区满了
        log_error("send error to client %d: %s", m_fd, strerror(errno));
        return false;
    }
    m_write_data.erase(0, sent);  // 发送成功就删除已发送的部分
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
