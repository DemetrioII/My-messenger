#include "../../../include/network/transport/server.hpp"

Server::Server(std::unique_ptr<ISocket> socket,
               std::unique_ptr<IAcceptor> acceptor_)
    : event_loop(std::make_unique<EventLoop>()),
      acceptHandler(std::make_shared<AcceptHandler>()),
      handler(std::make_shared<ServerHandler>()),
      socket_visitor(std::move(socket)), acceptor(std::move(acceptor_)) {}

void Server::process_pending_messages() {
  // Для фоновых задач
}

void Server::set_data_callback(
    std::function<void(int, std::vector<uint8_t>)> callback) {
  on_data_callback = callback;
}

void Server::start(int port) {
  socket_visitor->create_socket();
  if (socket_visitor->get_fd() != -1) {
    running = true;
    socket_visitor->setup_server(port, "127.0.0.1");
    socket_visitor->make_address_reusable();

    // acceptHandler = std::make_shared<AcceptHandler>();
    acceptHandler->init(shared_from_this(), std::move(acceptor));
    // handler = std::make_shared<ServerHandler>();
    handler->init(shared_from_this());
    event_loop->add_fd(socket_visitor->get_fd(), acceptHandler,
                       EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
    std::cout << "Сервер запущен на порту " << port << "\n";
  } else {
    std::runtime_error("Ошибка в создании сокета");
  }
}

void Server::run_event_loop() {
  while (running) {
    event_loop->run_once(100);
    process_pending_messages();
  }
}

void Server::stop() {
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

void Server::on_client_error(int fd) {}

void Server::on_client_connected(std::shared_ptr<IConnection> conn) {
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

void Server::on_client_disconnected(int fd) {
  std::cout << "Клиент " << fd << " отключился\n";
  if (event_loop) {
    event_loop->remove_fd(fd);
  }

  // Потом удалить всё остальное
  connection_manager.remove_connection(fd);

  // socket_visitor.close();
  // stop();
}

void Server::on_client_message(int fd, const std::vector<uint8_t> &data) {
  if (!data.empty()) {
    std::string message(data.begin(), data.end());
    if (message == "/exit")
      return;

    on_data_callback(fd, data);
  }
}

void Server::on_client_writable(int fd) {
  auto it = connection_manager.get_connection(fd);
  if (it == std::nullopt) {
    return;
  }
}

bool Server::is_running() const { return running; }

void Server::send(int fd, const std::vector<uint8_t> &raw_data) {
  connection_manager.send_to_buffer(fd, raw_data);
}

void Server::send(const std::string &message) {}

int Server::get_fd() const { return socket_visitor->get_fd(); }

Server::~Server() {
  std::cout << "~TCPServer()" << std::endl;
  stop();
}
