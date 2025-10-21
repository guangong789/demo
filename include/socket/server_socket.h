#pragma once
#include "socket.h"
#include "client_socket.h"

namespace gaozu {
    namespace socket {
        class ServerSocket : public test::socket::Socket {
        public:
            ServerSocket() = default;
            ServerSocket(const std::string &ip, int port);
            ~ServerSocket();

            std::shared_ptr<ClientSocket> accept_client();  // 封装连接fd
        };
    }
}