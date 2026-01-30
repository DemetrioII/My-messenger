#pragma once
#include "../protocol/framing.hpp"
#include "event_loop.hpp"
#include "handling.hpp"
#include "interface.hpp"
#include "raw_socket.hpp"
#include "transport.hpp"
#include <functional>
#include <iostream>
#include <vector>

class Client : public IClient, public std::enable_shared_from_this<Client> {
  TransportFabric transport_fabric;
  std::shared_ptr<ISocket> socket_visitor;
  std::unique_ptr<EventLoop> event_loop;
  std::shared_ptr<ClientHandler> handler;
  std::unique_ptr<ITransport> transport;
  std::unique_ptr<FramerMessage> framer;
  std::function<void(const std::vector<uint8_t> &)> on_data_callback;
  std::vector<uint8_t> recv_buffer; // Буфер для склейки сообщений

  struct sockaddr_in server_addr;

  Client(std::unique_ptr<ISocket> socket)
      : event_loop(std::make_unique<EventLoop>()),
        handler(std::make_shared<ClientHandler>()),
        socket_visitor(std::move(socket)) {}

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

  void set_data_callback(
      std::function<void(const std::vector<uint8_t> &)> f) override;

  bool connect(const std::string &server_ip, int port) override;

  void disconnect() override;

  void init_transport(std::unique_ptr<ITransport> transport_) override;

  void send_to_server(const std::vector<uint8_t> &data) override;

  void on_server_message() override;

  void run_event_loop() override;

  void on_writable(int fd) override;

  void on_disconnected() override;

  bool isConnected() const;

  int get_fd() const;

  ~Client() override;
};

class ClientFabric {
public:
  static std::shared_ptr<IClient>
  create_tcp_client(const std::string &server_ip, uint16_t port) {
    std::unique_ptr<ISocket> tcp_socket = std::make_unique<TCPSocket>();
    std::shared_ptr<Client> tcp_client = Client::create(std::move(tcp_socket));
    tcp_client->init_transport(TransportFabric::create_tcp());
    tcp_client->connect(server_ip, port);
    return tcp_client;
  }

  static std::shared_ptr<IClient>
  create_udp_client(const std::string &server_ip, uint16_t port) {
    std::unique_ptr<ISocket> udp_socket = std::make_unique<UDPSocket>();
    std::shared_ptr<Client> udp_client = Client::create(std::move(udp_socket));
    udp_client->connect(server_ip, port);
    auto server_addr = udp_client->get_addr();
    udp_client->init_transport(TransportFabric::create_udp(server_addr));
    return udp_client;
  }
};
