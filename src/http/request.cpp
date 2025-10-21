#include "http/request.h"
#include "utility/logger.h"
using namespace gaozu::logger;

bool Request::parse_from_str(const std::string& raw, size_t& pos, Request& out) {
    size_t method_end = raw.find(' ', pos);
    if (method_end == std::string::npos) return false;
    out.method = raw.substr(pos, method_end - pos);
    pos = method_end + 1;

    size_t path_end = raw.find(' ', pos);
    if (path_end == std::string::npos) return false;
    out.path = raw.substr(pos, path_end - pos);
    pos = path_end + 1;

    size_t line_end = raw.find('\r\n', pos);
    if (line_end == std::string::npos) return false;
    out.version = raw.substr(pos, line_end - pos);
    pos = line_end + 2;

    return true;
}

Request Request::parse(const std::string& raw) {
    Request req;
    size_t pos = 0;
    parse_from_str(raw, pos, req);
    // 解析header
    while (true) {
        size_t line_end = raw.find("\r\n", pos);
        if (line_end == std::string::npos) break;
        if (line_end == pos) {  // 空行表示header结束
            pos += 2;
            break;
        }
        std::string line = raw.substr(pos, line_end - pos);
        size_t colon = line.find(":");
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            if (!value.empty() && value[0] == ' ') value.erase(0, 1);
            req.headers[key] = value;
        }
        pos = line_end + 2;
    }
    // 解析body
    auto len_it = req.headers.find("Content-Length");
    if (len_it != req.headers.end()) {
        int len = std::stoi(len_it->second);
        if (pos + len <= raw.size()) {
            req.body = raw.substr(pos, len);
        }
    }
    // 解析Connection头
    req.keep_alive = true;  // 默认HTTP/1.1保持连接
    auto conn_it = req.headers.find("Connection");
    if (conn_it != req.headers.end()) {
        std::string conn_val = conn_it->second;
        std::transform(conn_val.begin(), conn_val.end(), conn_val.begin(), ::tolower);
        req.keep_alive = (conn_val != "close");
    }
    return req;
}

bool Request::is_complete(const std::string& buf) {
    auto pos = buf.find("\r\n\r\n");  // HTTP头部是否完整
    if (pos == std::string::npos) return false;
    auto it = buf.find("Content-Length:");  // 如果没有Content-Length头部，说明是一个没有消息体的请求
    if (it == std::string::npos) return true;
    size_t start = buf.find(":", it) + 1;
    size_t end = buf.find("\r\n", start);
    int content_len = std::stoi(buf.substr(start, end - start));
    size_t total_len = pos + 4 + content_len;
    bool complete = buf.find("\r\n\r\n") != std::string::npos;
    log_info("is_complete=%d, length=%zu, content=<<<%s>>>",
                            complete, buf.size(), buf.c_str());
    return buf.size() >= total_len;
}

int Request::get_total_length(const std::string &buf) {
    auto pos = buf.find("\r\n\r\n");
    if (pos == std::string::npos) return 0;

    auto it = buf.find("Content-Length:");
    int content_len = 0;
    if (it != std::string::npos) {
        size_t start = buf.find(":", it) + 1;
        size_t end = buf.find("\r\n", start);
        content_len = std::stoi(buf.substr(start, end - start));
    }
    return pos + 4 + content_len;
}