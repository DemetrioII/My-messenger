// acceptor.hpp
#pragma once
#include "connection.hpp"
#include "interface.hpp"
#include "transport.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

class Acceptor {
  TransportFabric transport_fabric;

public:
  Acceptor() = default;
  std::optional<std::shared_ptr<TCPConnection>> accept(int server_fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = ::accept(server_fd, (struct sockaddr *)&addr, &addr_len);
    if (client_fd == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Ошибка accept: " << strerror(errno) << std::endl;
      }
      return std::nullopt;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, O_NONBLOCK | flags);

    auto connection = std::make_shared<TCPConnection>(client_fd, addr);
    connection->init_transport(std::move(transport_fabric.create_tcp()));
    return connection;
  }

  ~Acceptor() = default;
};
