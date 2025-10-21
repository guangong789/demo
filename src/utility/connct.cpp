#include "utility/connct.h"

using namespace gaozu::logger;

Connct::Connct(int fd, bool close) : m_fd(fd), closed(close), keep_alive(false) {
    log_info("Connection created: %d", m_fd);
}

Connct::Connct(std::shared_ptr<ClientSocket> socket) : client(socket), m_fd(socket->get_fd()), keep_alive(false) {
    log_info("Connection created: %d", m_fd);
}

Connct::~Connct() {
    close();
}

int Connct::get_fd() const {
    return m_fd;
}

bool Connct::on_read() {
    std::lock_guard<std::mutex> lock(mtx);
    if (is_closed()) return false;

    char buf[4096];
    while (true) {
        ssize_t len = recv(m_fd, buf, sizeof(buf), 0);
        if (len > 0) {
            m_read_data.append(buf, len);
        } else if (len == 0) { // 客户端关闭
            return false;
        } else {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == ECONNRESET) return false;
            return false;
        }
    }
    return true;
}

bool Connct::send_data(const std::string& data) {
    if (data.empty()) return true;

    std::lock_guard<std::mutex> lock(mtx);
    if (is_closed()) return false;

    // 追加待发送数据
    m_write_data += data;

    // 尝试发送尽可能多的数据
    while (!m_write_data.empty()) {
        ssize_t sent = send(m_fd, m_write_data.data(), m_write_data.size(), 0);
        if (sent > 0) {
            m_write_data.erase(0, sent);
        } else if (sent == 0) {
            break;
        } else {
            if (errno == EINTR) continue;  // 被信号打断，重试
            if (errno == EAGAIN || errno == EWOULDBLOCK) {  // 内核缓冲区满，先不要关闭连接，等待 EPOLLOUT 通知
               
                break;
            }
            if (errno == ECONNRESET || errno == EPIPE) {
                log_info("Client %d closed connection during send", m_fd);
                return false;  // 真正的连接关闭
            }
            log_error("send_data failed for client %d: %s", m_fd, strerror(errno));
            return false;
        }
    }

    if (!m_write_data.empty()) {
        log_info("Partial data pending for client %d: %zu bytes left", m_fd, m_write_data.size());
    }
    return true;
}

bool Connct::on_write() {
    std::lock_guard<std::mutex> lock(mtx);
    if (is_closed() || m_write_data.empty()) return true;

    while (!m_write_data.empty()) {
        ssize_t sent = send(m_fd, m_write_data.data(), m_write_data.size(), 0);
        if (sent > 0) {
            m_write_data.erase(0, sent);
        } else if (sent == 0) {
            break;
        } else {
            if (errno == EINTR) continue;  // 被信号打断，重试
            if (errno == EAGAIN || errno == EWOULDBLOCK) {  // 内核缓冲区满，等待 EPOLLOUT
                break;
            }
            if (errno == ECONNRESET || errno == EPIPE) {
                log_info("Client %d closed connection during write", m_fd);
                return false;
            }
            log_error("on_write failed for client %d: %s", m_fd, strerror(errno));
            return false;
        }
    }

    if (!m_write_data.empty()) {
        log_info("Partial data still pending for client %d: %zu bytes", m_fd, m_write_data.size());
    }
    return true;
}

bool Connct::has_pending_data() const {
    return !m_write_data.empty();
}

void Connct::close() {
    bool expected = false;
    if (!closed.compare_exchange_strong(expected, true)) {
        return;
    }
    std::lock_guard<std::mutex> lock(mtx);
    if (client) {
        client->close_fd();
        client.reset();
    }
    log_info("Connection closed: %d", m_fd);
}

bool Connct::is_closed() const {
    return closed.load();
}

void Connct::change_closed() {
    std::lock_guard<std::mutex> lock(mtx);
    closed = true;
}

std::string& Connct::get_read_buf() {
    return m_read_data;
}

void Connct::clear_read_buf(size_t n) {
    if (n >= m_read_data.size()) m_read_data.clear();
    else m_read_data.erase(0, n);
}

bool Connct::get_keep_alive() const {
    return keep_alive;
}

void Connct::set_keep_alive(bool val) {
    keep_alive = val;
}