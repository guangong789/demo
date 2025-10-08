#pragma once
#include "socket.h"

namespace gaozu {
    namespace socket {
        class ClientSocket : public test::socket::Socket {
        public:
            ClientSocket() = default;
            ClientSocket(const std::string &ip, int port);
            ~ClientSocket() override;
        };
    }
}