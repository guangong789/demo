#include "utility/connct.h"
#include "utility/logger.h"

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

bool Connct::on_read() {
    char buf[1024] = {0};
    ssize_t len = recv(m_fd, buf, sizeof(buf)-1, 0);
    if (len < 0) {
        if (errno == EINTR) return true;  // EINTR: 系统调用中断
        log_error("recv error from client %d: errno=%d, errmsg=%s", m_fd, errno, strerror(errno));
        return false;
    } else if (len == 0) {  // 客户端关闭连接
        log_info("Client %d disconnected", m_fd);
        return false;
    } else {
        buf[len] = '\0';
        log_info("Received from client %d: %.100s", m_fd, buf);
        return send_data(std::string(buf, len));
    }
}

bool Connct::send_data(const std::string& data) {
    if (data.empty()) return true;  // 还没有数据
    if (!data.empty()) m_write_data += data;  // 有剩余数据就加入缓冲区
    ssize_t sent = send(m_fd, data.data(), data.length(), 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {  // 缓冲区写满了，先缓存数据
            m_write_data += data;
            return true;
        } else {
            log_error("Send error to client %d: %s", m_fd, strerror(errno));
            return false;
        }
    }
    if (sent < (ssize_t)data.length()) {  // 只发了一部分，就将没发的缓存起来 
        m_write_data += data.substr(sent);
    }
    return true;
}

bool Connct::on_write() {
    if(m_write_data.empty()) return true;
    ssize_t sent = send(m_fd, m_write_data.data(), m_write_data.length(), 0);
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
        log_error("on_write failed for client %d: %s", m_fd, strerror(errno));
        return false;
    } else {
        m_write_data.erase(0, sent);  // 删掉已发送的部分
        return true;
    }
}

bool Connct::has_pending_data() const {
    return !m_write_data.empty();
}