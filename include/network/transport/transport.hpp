// transport.hpp
#pragma once
#include "interface.hpp"
#include <cstring>
#include <iostream>
#include <memory.h>
#include <optional>
#include <type_traits>
#include <variant>

struct TCPTransport {
  int fd;
  std::vector<uint8_t> accumulation_buffer;

  TCPTransport(int FD) : fd(FD) {}

  ssize_t send(const std::vector<uint8_t> &data);

  ReceiveResult receive();

  ~TCPTransport();
};

struct UDPTransport {
  int fd;
  sockaddr_in addr;
  std::vector<uint8_t> accumulation_buffer;

  UDPTransport(int f, struct sockaddr_in address) : fd(f), addr(address) {}

  ssize_t send(const std::vector<uint8_t> &data);

  ReceiveResult receive();

  ~UDPTransport();
};

struct TLSTransport {
  int fd;
  SSL *ssl_;
  std::vector<uint8_t> accumulation_buffer;

  TLSTransport(int f, SSL *ssl_obj) : fd(f), ssl_(ssl_obj) {}

  ssize_t send(const std::vector<uint8_t> &data);

  ReceiveResult receive();

  ~TLSTransport();
};

enum class TransportType { TCP, UDP, TLS };

struct ITransport {
  std::variant<TCPTransport, UDPTransport, TLSTransport> transport;

  template <typename T, typename = std::enable_if_t<
                            !std::is_same_v<std::decay_t<T>, ITransport> &&
                            (std::is_same_v<std::decay_t<T>, TCPTransport> ||
                             std::is_same_v<std::decay_t<T>, UDPTransport> ||
                             std::is_same_v<std::decay_t<T>, TLSTransport>)>>

  ITransport(T &&t) : transport(std::forward<T>(t)) {}

  ssize_t send(const std::vector<uint8_t> &data) {
    return std::visit([&](auto &t) { return t.send(data); }, transport);
  }

  ReceiveResult receive() {
    return std::visit([&](auto &t) { return t.receive(); }, transport);
  }

  ~ITransport() { std::cout << "~ITransport()" << std::endl; }
};

class TransportFactory {
public:
  static ITransport create_tcp(int fd) { return ITransport(TCPTransport(fd)); }
  static ITransport create_udp(int fd, sockaddr_in addr) {
    return ITransport(UDPTransport(fd, addr));
  }
  static ITransport create_tls(int fd, SSL *ssl) {
    return ITransport(TLSTransport(fd, ssl));
  }
};
