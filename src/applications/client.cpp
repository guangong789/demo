#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <unistd.h>
#include "socket/client_socket.h"
#include "utility/log_init.h"
using namespace test::socket;
using namespace gaozu::logger;

int main(int argc, char* argv[]) {
    LoggerInitializer::create("logs/client.log")
        .set_console(true)
        .set_level(Logger::INFO)
        .set_max_size(1 * 1024)
        .set_backup_count(1);

    gaozu::socket::ClientSocket client("127.0.0.1", 8080);
    log_info("Connected to server 127.0.0.1:8080");

    std::atomic<bool> running(true);

    // 接收线程
    std::thread recv_thread([&]() {
        char buf[1024];
        while (running) {
            ssize_t len = client.recv(buf, sizeof(buf) - 1);
            if (len > 0) {
                buf[len] = '\0';
                std::cout << "\n[Server] " << buf << std::endl;
                std::cout << "> " << std::flush;  // 重新打印输入提示符
            } else if (len == 0) {
                log_info("Server closed connection");
                running = false;
                break;
            } else {
                if (errno == EINTR) continue;
                log_error("recv error: errno = %d, errmsg = %s", errno, strerror(errno));
                running = false;
                break;
            }
        }
    });

    // 发送线程（主线程）
    std::string input;
    std::cout << "> " << std::flush;
    while (running && std::getline(std::cin, input)) {
        if (input == "exit") {
            log_info("Client exiting...");
            running = false;
            break;
        }
        if (input.empty()) continue;
        ssize_t ret = client.send(input.c_str(), input.size());
        if (ret < 0) {
            log_error("send error: errno = %d, errmsg = %s", errno, strerror(errno));
            break;
        }
        std::cout << "> " << std::flush;
    }

    // 收尾
    running = false;
    client.close();
    if (recv_thread.joinable()) recv_thread.join();
    log_info("Client shutdown complete");
    return 0;
}
