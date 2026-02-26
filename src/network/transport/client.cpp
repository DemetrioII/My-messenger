#include "../../../include/network/transport/client.hpp"

Client::Client(std::unique_ptr<ISocket> socket)
    : event_loop(std::make_unique<EventLoop>()),
      handler(std::make_shared<ClientHandler>()),
      socket_visitor(std::move(socket)), transport(TransportType::TLS) {
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();

  ssl_ctx_ = SSL_CTX_new(TLS_client_method());
}

void Client::set_data_callback(
    std::function<void(const std::vector<uint8_t> &)> f) {
  on_data_callback = f;
}

bool Client::connect(const std::string &server_ip, int port) {
  handler->init(shared_from_this());

  if (socket_visitor->setup_connection(server_ip, port) < 0)
    return false;

  server_addr = socket_visitor->addr;
  event_loop->add_fd(socket_visitor->fd, handler, EPOLLIN | EPOLLRDHUP);
  tls_wrapper_ =
      std::make_unique<ClientTLSWrapper>(ssl_ctx_, socket_visitor->fd);
  transport =
      TransportFactory::create_tls(socket_visitor->fd, tls_wrapper_->ssl);
  return true;
}

void Client::tls_handshake() { tls_wrapper_->tls_handshake(); }

bool Client::tls_handshake_done() { return tls_wrapper_->handshake_done_; }

struct sockaddr_in Client::get_addr() { return server_addr; }

void Client::disconnect() {
  if (socket_visitor->fd != -1) {
    event_loop->remove_fd(socket_visitor->fd);
    ::shutdown(socket_visitor->fd, SHUT_RDWR);
    // socket_visitor->close(); // Вызовется в деструкторе Fd автоматически
  }
  if (ssl_ctx_) {
    SSL_CTX_free(ssl_ctx_);
    EVP_cleanup();
  }
}

void Client::send_to_server(const std::vector<uint8_t> &data) {
  // Добавляем заголовок длины, как того ждет сервер
  std::vector<uint8_t> packet;
  framer->form_message(data, packet);
  send(transport, packet);
}

void Client::on_server_message() {
  auto result = receive(transport);
  if (result.status == ReceiveStatus::OK && result.data.data()) {
    recv_buffer.insert(recv_buffer.end(), result.data.data(),
                       result.data.data() + result.data.length);

    // Обрабатываем все накопившиеся полные сообщения
    // while (recv_buffer.size() >= sizeof(uint32_t)) {
    while (framer->has_message_in_buffer(recv_buffer)) {
      /*  uint32_t len;
      std::memcpy(&len, recv_buffer.data(), sizeof(uint32_t));
      len = ntohl(len);
                      */

      // if (recv_buffer.size() < 4 + len) break; // Ждем досылки
      // if (!framer->has_message_in_buffer(recv_buffer)) break;

      // std::string message(recv_buffer.begin() + sizeof(uint32_t),
      // recv_buffer.begin() + sizeof(uint32_t) + len);
      std::vector<uint8_t> raw_bytes = framer->extract_message(recv_buffer);
      on_data_callback(raw_bytes);
      // std::string message(raw_bytes.begin(), raw_bytes.end());
      // std::cout << "\n[Сервер]: " << message << std::endl;
      // recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 +
      // len);
    }
  } else if (result.status == ReceiveStatus::CLOSED) {
    on_disconnected();
  }
}

void Client::run_event_loop() {
  std::cout << "Введите сообщение (или /exit): " << std::endl;
  while (isConnected()) {
    event_loop->run_once(50); // Небольшой таймаут, чтобы не грузить CPU
    // Внимание: std::cin блокирует поток!
    // Для тестов это ок, но сообщения от сервера будут видны только ПОСЛЕ
    // ввода
  }
}

void Client::on_writable(int fd) {}
void Client::on_disconnected() {
  std::cout << "Соединение разорвано" << std::endl;
}
bool Client::isConnected() const { return socket_visitor->fd != -1; }
int Client::get_fd() const { return socket_visitor->fd; }
Client::~Client() { disconnect(); }
