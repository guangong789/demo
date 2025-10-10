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
            ~Socket();

            int get_fd() const;

            bool bind(const std::string& ip, int port);  // 将socket绑定到指定的IP地址和端口号
            bool listen(int backlog);  // backlog: 等待连接队列的最大长度(用于服务端)
            bool connect(const std::string& ip, int port);  // 连接到指定的服务器
            virtual int accept();  // 接受客户端的连接请求
            int send(const char* buf, int len);
            int recv(char* buf, int len);  // len: 接收缓冲区长度
            void close();

            // socket常用属性
            bool set_blocking();
            bool set_non_blocking();  // 由阻塞IO变成非阻塞IO
            bool set_send_buffer(int size);  // 设置发送缓冲区大小
            bool set_recv_buffer(int size);  // 设置接收缓冲区
            bool set_linger(bool active, int seconds);  // 设置socket关闭时的行为
            // 禁用时:立即返回，后台发送剩余数据; 启用时:在指定时间内尝试发送剩余数据，超时则丢弃
            bool set_keepalive();  // TCP保活机制
            bool set_reuseaddr();  // 允许重用本地地址,服务器重启后可以立即绑定相同端口

        protected:
            std::string m_ip;
            int m_port;
            int m_sockfd;
        };
    }
}