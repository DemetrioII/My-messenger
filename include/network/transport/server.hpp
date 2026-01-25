// server.hpp
#pragma once
#include "acceptor.hpp"
#include "event_loop.hpp"
#include "handling.hpp"
#include "interface.hpp"
#include "raw_socket.hpp"
#include <atomic>
#include <functional>
#include <mutex>

class Server : public IServer, public std::enable_shared_from_this<Server> {
  Acceptor acceptor;
  std::atomic<bool> running{false};

  std::unique_ptr<EventLoop> event_loop;
  struct epoll_event ev, events[MAX_EVENTS];

  ConnectionManager connection_manager;
  // std::mutex server_mutex;

  std::shared_ptr<AcceptHandler> acceptHandler;
  std::shared_ptr<ServerHandler> handler;

  std::function<void(int fd, std::vector<uint8_t>)> on_data_callback;

private:
  std::unique_ptr<ISocket> socket_visitor;
  Server(std::unique_ptr<ISocket> socket);

  void process_pending_messages();

public:
  static std::shared_ptr<Server> create(SocketType type = SocketType::TCP) {
    std::unique_ptr<ISocket> socket;
    switch (type) {
    case SocketType::TCP:
      socket = std::make_unique<TCPSocket>();
      break;
    case SocketType::UDP:
      socket = std::make_unique<UDPSocket>();
      break;
    default:
      throw std::invalid_argument("Unknown socket type");
    }
    struct make_shared_enabler : public Server {
      make_shared_enabler(std::unique_ptr<ISocket> sock)
          : Server(std::move(sock)) {}
    };
    return std::make_shared<make_shared_enabler>(std::move(socket));
  }

  // Удалить копирование
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  void
  set_data_callback(std::function<void(int, std::vector<uint8_t>)> callback);

  void start(int port) override;

  void run_event_loop();

  void stop() override;

  void on_client_error(int fd) override;

  void on_client_connected(std::shared_ptr<ClientConnection> conn) override;

  void on_client_disconnected(int fd) override;

  void on_client_message(int fd, const std::vector<uint8_t> &data) override;

  void on_client_writable(int fd) override;

  bool is_running() const override;

  void send(int fd, const std::vector<uint8_t> &raw_data);

  void send(const std::string &message);

  int get_fd() const;

  ~Server() override;
};
