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

  std::optional<std::shared_ptr<ClientConnection>> accept(int server_fd);

  ~Acceptor() = default;
};
