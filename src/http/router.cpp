#include "http/router.h"

void Router::add_route(const std::string& path, Handler h) {
    m_routes[path] = std::move(h);
}

Response Router::handler(const Request& req) const {
    auto it = m_routes.find(req.path);
    if (it != m_routes.end()) {
        return (it->second)(req);
    } else {
        return Response::make_text(404, "Not Found");
    }
}