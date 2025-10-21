#include <iostream>
#include "http/request.h"
#include "http/response.h"
#include "http/router.h"

int main() {
    std::string raw_req =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: test\r\n"
        "Accept: */*\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    Request req = Request::parse(raw_req);
    std::cout << "Parsed Request:\n";
    std::cout << "  method: " << req.method << "\n";
    std::cout << "  path: " << req.path << "\n";
    std::cout << "  keep_alive: " << req.keep_alive << "\n";

    // 注册路由
    Router& router = Router::instance();
    router.add_route("/hello", [](const Request& req) {
        return Response::make_text(200, "Hello from Router!");
    });

    router.add_route("/json", [](const Request& req) {
        return Response::make_json(200, R"({"msg":"ok"})");
    });

    // 测试路由分发
    Response res = router.handler(req);
    std::cout << "\nResponse:\n" << res.serialize(req.keep_alive) << std::endl;

    return 0;
}
