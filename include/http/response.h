#pragma once
#include <string>
#include <unordered_map>
#include <sstream>

struct Response {
    int status_code = 200;
    std::string status_msg = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string serialize(bool keep_alive = false) const;
    static std::string status_func(int code);
    static Response make_text(int code, const std::string& body, const std::string& Content_Type = "text/plain");
    static Response make_html(int code, const std::string& body);
    static Response make_json(int code, const std::string& json_body);
    std::string to_string() const;
};