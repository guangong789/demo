#include "socket/client_socket.h"
using namespace gaozu::socket;

ClientSocket::ClientSocket(const std::string &ip, int port) : Socket() {
    connect(ip, port);
}

ClientSocket::~ClientSocket() {}