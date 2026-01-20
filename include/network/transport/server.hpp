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
  Server(std::unique_ptr<ISocket> socket)
      : event_loop(std::make_unique<EventLoop>()),
        acceptHandler(std::make_shared<AcceptHandler>()),
        handler(std::make_shared<ServerHandler>()),
        socket_visitor(std::move(socket)) {}

  void process_pending_messages() {
    // Для фоновых задач
  }

public:
  enum class SocketType { TCP, UDP };

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
  set_data_callback(std::function<void(int, std::vector<uint8_t>)> callback) {
    on_data_callback = callback;
  }

  void start(int port) override {
    socket_visitor->create_socket();
    if (socket_visitor->get_fd() != -1) {
      running = true;
      socket_visitor->setup_server(port, "127.0.0.1");
      socket_visitor->make_address_reusable();

      // acceptHandler = std::make_shared<AcceptHandler>();
      acceptHandler->init(shared_from_this(), acceptor);
      // handler = std::make_shared<ServerHandler>();
      handler->init(shared_from_this());
      event_loop->add_fd(socket_visitor->get_fd(), acceptHandler,
                         EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
      std::cout << "Сервер запущен на порту " << port << "\n";
    } else {
      std::runtime_error("Ошибка в создании сокета");
    }
  }

  void run_event_loop() {
    while (running) {
      event_loop->run_once(100);
      process_pending_messages();
    }
  }

  void stop() override {
    std::cout << "Сервер отключается" << std::endl;
    if (!running)
      return; // Защита от повторного вызова

    running = false;

    // 1. Пробуждаем цикл, чтобы он увидел running = false
    if (event_loop) {
      event_loop->stop();
    }

    // 2. Закрываем слушающий сокет
    // Используйте только один метод закрытия!
    // socket_visitor.close() внутри Fd сделает всё остальное.
    socket_visitor->close();
  }

  void on_client_error(int fd) override {}

  void on_client_connected(std::shared_ptr<ClientConnection> conn) override {
    // std::lock_guard<std::mutex> lock(server_mutex);
    std::cout << "Новое подключение от " << conn->get_fd() << '\n'
              << "IP=" << inet_ntoa(conn->get_addr().sin_addr) << ":"
              << ntohs(conn->get_addr().sin_port) << std::endl;

    int fd = conn->get_fd();

    connection_manager.add_connection(fd, conn);
    handler->add_client(fd, conn);
    event_loop->add_fd(fd, handler,
                       EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
  }

  void on_client_disconnected(int fd) override {
    std::cout << "Клиент " << fd << " отключился\n";
    if (event_loop) {
      event_loop->remove_fd(fd);
    }

    // Потом удалить всё остальное
    connection_manager.remove_connection(fd);

    // socket_visitor.close();
    // stop();
  }

  void on_client_message(int fd, const std::vector<uint8_t> &data) override {
    if (!data.empty()) {
      std::string message(data.begin(), data.end());
      if (message == "/exit")
        return;

      on_data_callback(fd, data);
    }
  }

  void on_client_writable(int fd) override {
    auto it = connection_manager.get_connection(fd);
    if (it == std::nullopt) {
      return;
    }
  }

  bool is_running() const override { return running; }

  void send(int fd, const std::vector<uint8_t> &raw_data) {
    connection_manager.send_to_buffer(fd, raw_data);
  }

  void send(const std::string &message) {}

  int get_fd() const { return socket_visitor->get_fd(); }

  ~Server() override {
    std::cout << "~TCPServer()" << std::endl;
    stop();
  }
};
