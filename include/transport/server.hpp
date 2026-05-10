// server.hpp
#pragma once
#include "acceptor.hpp"
#include "event_loop.hpp"
#include "handling.hpp"
#include "interface.hpp"
#include "raw_socket.hpp"
#include "tls.hpp"
#include <atomic>
#include <functional>
#include <mutex>

class Server : public IServer, public std::enable_shared_from_this<Server> {
  std::atomic<bool> running{false};
  std::unique_ptr<IAcceptor> acceptor;

  std::unique_ptr<EventLoop> event_loop;
  struct epoll_event ev, events[MAX_EVENTS];

  ConnectionManager connection_manager;
  // std::mutex server_mutex;

  std::shared_ptr<AcceptHandler> acceptHandler;
  std::shared_ptr<ServerHandler> handler;

  std::unordered_map<int, std::unique_ptr<ServerTLSWrapper>> tls_wrapper_;

  SSL_CTX *ssl_ctx_;

  std::function<void(int fd, std::vector<uint8_t>)> on_data_callback;
  std::function<void(int fd)> on_disconnected_callback;

private:
  std::unique_ptr<ISocket> socket_;
  Server(std::unique_ptr<ISocket> socket, std::unique_ptr<IAcceptor> acceptor_);

  void process_pending_messages();

public:
  static std::shared_ptr<Server> create(std::unique_ptr<ISocket> &socket_,
                                        std::unique_ptr<IAcceptor> &acceptor_) {
    std::unique_ptr<ISocket> socket;
    std::unique_ptr<IAcceptor> accept;

    socket = std::move(socket_);
    accept = std::move(acceptor_);

    struct make_shared_enabler : public Server {
      make_shared_enabler(std::unique_ptr<ISocket> sock,
                          std::unique_ptr<IAcceptor> acc)
          : Server(std::move(sock), std::move(acc)) {}
    };
    return std::make_shared<make_shared_enabler>(std::move(socket),
                                                 std::move(accept));
  }

  // Удалить копирование
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  void set_data_callback(
      std::function<void(int, std::vector<uint8_t>)> data_callback,
      std::function<void(int)> disconnect_callback) override;

  void start(const std::string &ip, int port) override;

  void run_event_loop() override;

  void stop() override;

  void tls_handshake(int fd) override;

  bool tls_handshake_done(int fd) override;

  void on_client_error(int fd) override;

  void on_client_connected(std::shared_ptr<IConnection> conn) override;

  void on_client_disconnected(int fd) override;

  void on_client_message(int fd, const std::vector<uint8_t> &data) override;

  void on_client_writable(int fd) override;

  bool is_running() const override;

  void send(int fd, const std::vector<uint8_t> &raw_data) override;

  void send(const std::string &message);

  int get_fd() const;

  ~Server() override;
};

class ServerFactory {
public:
  static std::shared_ptr<Server> tcp_server([[maybe_unused]] const std::string &ip,
                                            [[maybe_unused]] int port) {
    auto tcp_socket = std::make_unique<ISocket>(SocketType::TCP);
    std::unique_ptr<IAcceptor> tcp_acceptor = std::make_unique<TCPAcceptor>();
    auto server = Server::create(tcp_socket, tcp_acceptor);
    // server->start(port);
    return server;
  }

  static std::shared_ptr<Server> udp_server([[maybe_unused]] const std::string &ip,
                                            [[maybe_unused]] int port) {
    auto udp_socket = std::make_unique<ISocket>(SocketType::UDP);
    std::unique_ptr<IAcceptor> udp_acceptor = std::make_unique<UDPAcceptor>();
    auto server = Server::create(udp_socket, udp_acceptor);
    // server->start(port);
    return server;
  }
};
