#pragma once
#include <mutex>
#include "socket.h"
#include "utility/logger.h"

namespace gaozu {
    namespace socket {
        class ClientSocket : public test::socket::Socket {
        public:
            ClientSocket() = default;
            ClientSocket(int fd);
            ClientSocket(const std::string &ip, int port, bool non_blocking = true);
            ~ClientSocket();

            void close_fd();
            bool set_non_blocking(bool non_blocking = true);
        private:
            bool m_non_blocking = true;
            std::mutex mtx;
        };
    }
}