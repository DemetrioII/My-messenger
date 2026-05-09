#include "../../../include/network/transport/server.hpp"
#include "../../../include/utils/logger.hpp"

Server::Server(std::unique_ptr<ISocket> socket,
               std::unique_ptr<IAcceptor> acceptor_)
    : event_loop(std::make_unique<EventLoop>()),
      acceptHandler(std::make_shared<AcceptHandler>()),
      handler(std::make_shared<ServerHandler>()), socket_(std::move(socket)),
      acceptor(std::move(acceptor_)) {
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();

  ssl_ctx_ = SSL_CTX_new(TLS_server_method());
  if (!ssl_ctx_) {
    throw std::runtime_error("TLS context initialization failed!");
  }

  if (SSL_CTX_use_certificate_file(ssl_ctx_, "cert.pem", SSL_FILETYPE_PEM) <=
          0 ||
      SSL_CTX_use_PrivateKey_file(ssl_ctx_, "key.pem", SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    throw std::runtime_error("TLS certificate load failed!");
  }
}

void Server::process_pending_messages() {
  // Для фоновых задач
}

void Server::set_data_callback(
    std::function<void(int, std::vector<uint8_t>)> callback,
    std::function<void(int)> disconnect_callback) {
  on_data_callback = callback;
  on_disconnected_callback = disconnect_callback;
}

void Server::start(const std::string &ip, int port) {
  socket_->setup_server(ip, port);
  if (socket_->fd != -1) {
    running = true;
    // acceptHandler = std::make_shared<AcceptHandler>();
    acceptHandler->init(shared_from_this(), std::move(acceptor));
    // handler = std::make_shared<ServerHandler>();
    handler->init(shared_from_this());
    event_loop->add_fd(socket_->fd, acceptHandler,
                       EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
    messenger::log::info("Server started on port " + std::to_string(port));
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
  messenger::log::info("Server stopping");
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
  // socket_->close();
}

void Server::on_client_error(int fd) {}

bool Server::tls_handshake_done(int fd) {
  return tls_wrapper_[fd]->handshake_done_;
}

void Server::tls_handshake(int fd) { tls_wrapper_[fd]->tls_handshake(); }

void Server::on_client_connected(std::shared_ptr<IConnection> conn) {
  // std::lock_guard<std::mutex> lock(server_mutex);
  messenger::log::info("New connection fd=" + std::to_string(conn->get_fd()));

  int fd = conn->get_fd();

  connection_manager.add_connection(fd, conn);
  handler->add_client(fd, conn);
  event_loop->add_fd(fd, handler,
                     EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP);

  tls_wrapper_[fd] = std::make_unique<ServerTLSWrapper>(ssl_ctx_, fd);

  conn->init_transport(
      std::move(TransportFactory::create_tls(fd, tls_wrapper_[fd]->ssl)));
}

void Server::on_client_disconnected(int fd) {
  messenger::log::info("Client disconnected fd=" + std::to_string(fd));
  if (event_loop) {
    event_loop->remove_fd(fd);
  }

  on_disconnected_callback(fd);

  // Потом удалить всё остальное
  connection_manager.remove_connection(fd);

  tls_wrapper_.erase(fd);

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
  auto done = it->get()->flush();
  if (done)
    event_loop->disable_write(fd);
}

bool Server::is_running() const { return running; }

void Server::send(int fd, const std::vector<uint8_t> &raw_data) {
  connection_manager.send_to_buffer(fd, raw_data);
  event_loop->enable_write(fd);
  // connection_manager.get_connection(fd)->get()->flush();
}

void Server::send(const std::string &message) {}

int Server::get_fd() const { return socket_->fd; }

Server::~Server() {
  messenger::log::debug("~TCPServer()");
  if (ssl_ctx_) {
    SSL_CTX_free(ssl_ctx_);
    EVP_cleanup();
  }
  stop();
}
