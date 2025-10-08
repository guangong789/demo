#pragma once
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "utility/logger.h"

namespace test {
    namespace socket {
        class Socket {
        public:
            Socket();
            Socket(int sockfd);
            virtual ~Socket();

            int get_fd() const;

            bool bind(const std::string& ip, int port);
            bool listen(int backlog);
            bool connect(const std::string& ip, int port);
            virtual int accept();
            int send(const char* buf, int len);
            int recv(char* buf, int len);
            void close();

            // socket常用属性
            bool set_blocking();
            bool set_non_blocking();  // 由阻塞IO变成非阻塞IO
            bool set_send_buffer(int size);  // 设置发送缓冲区大小
            bool set_recv_buffer(int size);  // 设置接收缓冲区
            bool set_linger(bool active, int seconds);
            bool set_keepalive();
            bool set_reuseaddr();

        protected:
            std::string m_ip;
            int m_port;
            int m_sockfd;
        };
    }
}