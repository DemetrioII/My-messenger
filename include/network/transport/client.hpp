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

  Client(std::unique_ptr<ISocket> socket)
      : event_loop(std::make_unique<EventLoop>()),
        handler(std::make_shared<ClientHandler>()),
        socket_visitor(std::move(socket)) {}

public:
  static std::shared_ptr<Client> create(SocketType type = SocketType::TCP) {
    std::unique_ptr<ISocket> socket;
    switch (type) {
    case SocketType::TCP:
      socket = std::make_unique<TCPSocket>();
      break;
    case SocketType::UDP:
      socket = std::make_unique<UDPSocket>();
      break;
    }
    struct make_shared_enabler : public Client {
      make_shared_enabler(std::unique_ptr<ISocket> sock)
          : Client(std::move(sock)) {}
    };
    return std::make_shared<make_shared_enabler>(std::move(socket));
  }
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
