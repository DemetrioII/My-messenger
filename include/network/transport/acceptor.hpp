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

struct PeerSession;

class TCPAcceptor : public IAcceptor {
public:
  TCPAcceptor() = default;

  std::optional<std::shared_ptr<IConnection>> accept(int server_fd) override;

  ~TCPAcceptor() override;
};

class UDPAcceptor : public IAcceptor {
public:
  UDPAcceptor() = default;

  std::optional<std::shared_ptr<IConnection>> accept(int server_fd) override;

  ~UDPAcceptor() override;
};

class IPeerAcceptor {
public:
  virtual std::optional<std::shared_ptr<PeerSession>> accept(int fd) = 0;
  virtual ~IPeerAcceptor() = default;
};

class PeerTCPAcceptor : public IPeerAcceptor {
public:
  std::optional<std::shared_ptr<PeerSession>> accept(int fd) override;

  ~PeerTCPAcceptor() override;
};

class PeerUDPAcceptor : public IPeerAcceptor {
public:
  std::optional<std::shared_ptr<PeerSession>> accept(int fd) override;

  ~PeerUDPAcceptor() override;
};
