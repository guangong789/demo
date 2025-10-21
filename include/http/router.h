#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "request.h"
#include "response.h"

class Router {
public:
    using Handler = std::function<Response(const Request&)>;

    static Router& instance() {
        static Router r;
        return r;
    }

    void add_route(const std::string& path, Handler h);
    Response handler(const Request& req) const;
private:
    std::unordered_map<std::string, Handler> m_routes;
};