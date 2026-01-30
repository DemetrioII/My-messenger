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

class TCPAcceptor : public IAcceptor {
public:
  TCPAcceptor() = default;

  std::optional<std::shared_ptr<ClientConnection>>
  accept(int server_fd) override;

  ~TCPAcceptor() override;
};

class UDPAcceptor : public IAcceptor {
public:
  UDPAcceptor() = default;

  std::optional<std::shared_ptr<ClientConnection>>
  accept(int server_fd) override;

  ~UDPAcceptor() override;
};
