#pragma once
#include "../protocol/framing.hpp"
#include "event_loop.hpp"
#include "handling.hpp"
#include "interface.hpp"
#include "raw_socket.hpp"
#include "tls.hpp"
#include "transport.hpp"
#include <functional>
#include <iostream>
#include <vector>

// using ITransport = std::variant<TCPTransport, UDPTransport, TLSTransport>;

class Client : public IClient, public std::enable_shared_from_this<Client> {
  std::shared_ptr<ISocket> socket_visitor;
  std::unique_ptr<EventLoop> event_loop;
  std::shared_ptr<ClientHandler> handler;
  std::unique_ptr<ITransport> transport;
  std::unique_ptr<FramerMessage> framer;
  std::function<void(const std::vector<uint8_t> &)> on_data_callback;
  std::vector<uint8_t> recv_buffer; // Буфер для склейки сообщений

  struct sockaddr_in server_addr;

  SSL_CTX *ssl_ctx_;

  std::unique_ptr<ClientTLSWrapper> tls_wrapper_;

  Client(std::unique_ptr<ISocket> socket);

public:
  static std::shared_ptr<Client> create(std::unique_ptr<ISocket> socket_) {
    std::unique_ptr<ISocket> socket;
    socket = std::move(socket_);

    struct make_shared_enabler : public Client {
      make_shared_enabler(std::unique_ptr<ISocket> sock)
          : Client(std::move(sock)) {}
    };
    return std::make_shared<make_shared_enabler>(std::move(socket));
  }
  struct sockaddr_in get_addr();

  void tls_handshake() override;

  bool tls_handshake_done() override;

  void set_data_callback(
      std::function<void(const std::vector<uint8_t> &)> f) override;

  bool connect(const std::string &server_ip, int port) override;

  void disconnect() override;

  void send_to_server(const std::vector<uint8_t> &data) override;

  void on_server_message() override;

  void run_event_loop() override;

  void on_writable(int fd) override;

  void on_disconnected() override;

  bool isConnected() const;

  int get_fd() const;

  ~Client() override;
};

class ClientFactory {
public:
  static std::shared_ptr<Client> tcp_client(const std::string &ip, int port) {
    auto tcp_socket = std::make_unique<ISocket>(SocketType::TCP);
    auto client = Client::create(std::move(tcp_socket));
    client->connect(ip, port);
    return client;
  }

  static std::shared_ptr<Client> udp_client(const std::string &ip, int port) {
    auto udp_socket = std::make_unique<ISocket>(SocketType::UDP);
    auto client = Client::create(std::move(udp_socket));
    client->connect(ip, port);
    return client;
  }
};
