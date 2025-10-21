#include "http/response.h"
#include "utility/logger.h"
using namespace gaozu::logger;

std::string Response::serialize(bool keep_alive) const {
    std::string res = "HTTP/1.1 " + std::to_string(status_code) + " " + status_msg + "\r\n";

    // Content-Length
    res += "Content-Length: " + std::to_string(body.size()) + "\r\n";

    // Connection
    res += "Connection: " + std::string(keep_alive ? "keep-alive" : "close") + "\r\n";

    // Content-Type，优先使用 headers 中设置的，没有则默认 text/plain
    auto it = headers.find("Content-Type");
    if (it != headers.end()) {
        res += "Content-Type: " + it->second + "\r\n";
    } else {
        res += "Content-Type: text/plain\r\n";
    }

    // 其它 headers
    for (const auto& [key, value] : headers) {
        if (key != "Content-Type") {
            res += key + ": " + value + "\r\n";
        }
    }

    res += "\r\n";  // header 与 body 分隔

    res += body;

    log_info("Response serialized (%zu bytes):\n<<<%s>>>", res.size(), res.c_str());
    return res;
}


std::string Response::status_func(int code){
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default:  return "Unknown";
    }
}

Response Response::make_text(int code, const std::string& body, const std::string& Content_Type) {
    Response resp;
    resp.status_code = code;
    resp.status_msg = status_func(code);
    resp.body = body;
    resp.headers["Content-Type"] = Content_Type + "; charset=utf-8";
    return resp;
}

Response Response::make_html(int code, const std::string& body) {
    Response resp;
    resp.status_code = code;
    resp.status_msg = status_func(code);
    resp.body = body;
    resp.headers["Content-Type"] = "text/html; charset=utf-8";
    return resp;
}

Response Response::make_json(int code, const std::string& json_body) {
    Response resp;
    resp.status_code = code;
    resp.status_msg = status_func(code);
    resp.body = json_body;
    resp.headers["Content-Type"] = "application/json; charset=utf-8";
    return resp;
}

std::string Response::to_string() const {
    std::ostringstream oss;
    oss << "[HTTP Response] " << status_code << " " << status_msg
        << ", body_len=" << body.size();
    return oss.str();
}