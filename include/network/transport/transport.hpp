// transport.hpp
#pragma once
#include "interface.hpp"
#include <cstring>
#include <memory.h>
#include <optional>
#include <variant>

class TCPTransport : public ITransport {
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
};

class TransportFabric {
public:
  static std::unique_ptr<TCPTransport> create_tcp() {
    return std::make_unique<TCPTransport>(TCPTransport());
  }

  static std::unique_ptr<UDPTransport> create_udp(struct sockaddr_in &addr) {
    return std::make_unique<UDPTransport>(UDPTransport(addr));
  }

  static std::unique_ptr<TLSTransport> create_tls(SSL *ssl) {
    return std::make_unique<TLSTransport>(ssl);
  }
};
