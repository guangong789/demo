#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>

struct Request {
    std::string method;  // HTTP方法
    std::string path;  // 请求路径
    std::string version;
    std::unordered_map<std::string, std::string> headers;  // HTTP头部字段的键值对映射
    std::string body;  // 请求体内容
    bool keep_alive;

    static bool parse_from_str(const std::string& raw, size_t& pos, Request& out);
    static Request parse(const std::string& raw);
    static bool is_complete(const std::string& buf);
    static int get_total_length(const std::string &buf);
};