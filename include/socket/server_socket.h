#pragma once
#include "socket.h"

namespace gaozu {
    namespace socket {
        class ServerSocket : public test::socket::Socket {
        public:
            ServerSocket() = default;
            ServerSocket(const std::string &ip, int port);
            ~ServerSocket() override;

            int accept() override;
        };
    }
}