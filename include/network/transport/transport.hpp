// transport.hpp
#pragma once
#include "interface.hpp"
#include <cstring>
#include <iostream>
#include <memory.h>
#include <optional>
#include <variant>

/*class TCPTransport : public ITransport {
  std::optional<std::vector<uint8_t>>
  extract_complete_message(int fd, std::vector<uint8_t> &buffer) const;

public:
  TCPTransport() = default;

  ssize_t send(int fd, const std::vector<uint8_t> &data) const override;

  ReceiveResult receive(int fd) const override;

  void connect(int fd) override;

  ~TCPTransport() override;
};

class UDPTransport : public ITransport {
  sockaddr_in peer_addr;

public:
  UDPTransport(const sockaddr_in &addr);

  ssize_t send(int fd, const std::vector<uint8_t> &data) const override;

  ReceiveResult receive(int fd) const override;

  void connect(int fd) override;

  ~UDPTransport() override;
};

class TLSTransport : public ITransport {
  std::unique_ptr<ITransport> inner_;
  SSL *ssl_;

  std::optional<std::vector<uint8_t>>
  extract_complete_message(std::vector<uint8_t> &buffer) const;

public:
  TLSTransport(SSL *ssl) : ssl_(ssl) {}

  ssize_t send(int fd, const std::vector<uint8_t> &data) const override;

  ReceiveResult receive(int fd) const override;

  void connect(int fd) override;

  ~TLSTransport();
}; */

struct TCPTransport {
  int fd;

  TCPTransport(int FD) : fd(FD) {}

  ssize_t send(const std::vector<uint8_t> &data);

  ReceiveResult receive();

  ~TCPTransport();
};

struct UDPTransport {
  int fd;
  sockaddr_in addr;

  UDPTransport(int f, struct sockaddr_in address) : fd(f), addr(address) {}

  ssize_t send(const std::vector<uint8_t> &data);

  ReceiveResult receive();

  ~UDPTransport();
};

struct TLSTransport {
  int fd;
  SSL *ssl_;

  TLSTransport(int f, SSL *ssl_obj) : fd(f), ssl_(ssl_obj) {}

  ssize_t send(const std::vector<uint8_t> &data);

  ReceiveResult receive();

  ~TLSTransport();
};

enum class TransportType { TCP, UDP, TLS };

struct ITransport {
  TransportType type;

  union {
    TCPTransport tcp;
    UDPTransport udp;
    TLSTransport tls;
  };

  ITransport(TransportType t) { type = t; }

  ~ITransport() { std::cout << "~ITransport()" << std::endl; }
};

ssize_t send(ITransport &transport, const std::vector<uint8_t> &data);

ReceiveResult receive(ITransport &transport);

class TransportFactory {
public:
  static ITransport create_tcp(int fd) {
    ITransport interface(TransportType::TCP);
    interface.tcp = TCPTransport(fd);
    return interface;
  }
  static ITransport create_udp(int fd, sockaddr_in addr) {
    ITransport interface(TransportType::UDP);
    interface.udp = UDPTransport(fd, addr);
    return interface;
  }
  static ITransport create_tls(int fd, SSL *ssl) {
    ITransport interface(TransportType::TLS);
    interface.tls = TLSTransport(fd, ssl);
    return interface;
  }
};
