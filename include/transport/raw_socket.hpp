// raw_socket.hpp
#pragma once
#include "interface.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 10

/*class TCPSocket : public ISocket {
  Fd fd;

  struct sockaddr_in address;

public:
  int create_socket() override;

  void make_address_reusable() override;

  void setup_server(uint16_t PORT, const std::string &ip) override;

  int setup_connection(const std::string &ip, uint16_t port) override;

  sockaddr_in get_peer_address() const override;

  int get_fd() const override;

  void close() override;

  SocketType get_type() const override;

  bool check_connection_complete(int timeout_ms) override;

  int get_socket_error() const override;

  ~TCPSocket() override;
};

class UDPSocket : public ISocket {
  Fd fd;

  struct sockaddr_in address;

public:
  int create_socket() override;

  void setup_server(uint16_t port, const std::string &ip) override;

  void make_address_reusable() override;

  int setup_connection(const std::string &ip, uint16_t port) override;

  sockaddr_in get_peer_address() const override;

  SocketType get_type() const override;

  bool check_connection_complete(int timeout_ms) override;

  int get_socket_error() const override;

  int get_fd() const override;

  void close() override;

  ~UDPSocket() override;
};*/

struct TCPSocket {
  int fd{-1};

  sockaddr_in addr;

  TCPSocket() : fd(-1) {}

  ~TCPSocket() {
    if (fd != -1)
      ::close(fd);
  }

  void server(const std::string &ip, uint16_t port);

  int client(const std::string &ip, uint16_t port);

  bool check_connection_complete(int timeout_ms);
};

struct UDPSocket {
  int fd{-1};

  sockaddr_in addr;

  UDPSocket() : fd(-1) {}

  ~UDPSocket() {
    if (fd != -1)
      ::close(fd);
  }

  void server(const std::string &ip, uint16_t port);

  int client(const std::string &ip, uint16_t port);

  bool check_connection_complete(int timeout_ms);
};

struct ISocket {
  SocketType type;
  int fd;
  sockaddr_in addr;

  union {
    TCPSocket tcp;
    UDPSocket udp;
  };

  ISocket(SocketType t) : type(t) {
    if (t == SocketType::TCP)
      new (&tcp) TCPSocket();
    else
      new (&udp) UDPSocket();
  }

  ~ISocket() {}

  void setup_server(const std::string &ip, uint16_t port);

  int setup_connection(const std::string &ip, uint16_t port);

  bool check_connection_complete(int timeout_ms);
};
