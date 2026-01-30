#include "../../../include/network/transport/client.hpp"

void Client::set_data_callback(
    std::function<void(const std::vector<uint8_t> &)> f) {
  on_data_callback = f;
}

bool Client::connect(const std::string &server_ip, int port) {
  handler->init(shared_from_this());
  if (socket_visitor->create_socket() == -1)
    return false;
  if (socket_visitor->setup_connection(server_ip, port) < 0)
    return false;

  server_addr = socket_visitor->get_peer_address();
  event_loop->add_fd(socket_visitor->get_fd(), handler, EPOLLIN | EPOLLRDHUP);
  return true;
}

struct sockaddr_in Client::get_addr() { return server_addr; }

void Client::init_transport(std::unique_ptr<ITransport> transport_) {
  transport = std::move(transport_);
  transport->connect(socket_visitor->get_fd());
  framer = std::make_unique<FramerMessage>();
}

void Client::disconnect() {
  if (socket_visitor->get_fd() != -1) {
    event_loop->remove_fd(socket_visitor->get_fd());
    ::shutdown(socket_visitor->get_fd(), SHUT_RDWR);
    socket_visitor->close(); // Вызовется в деструкторе Fd автоматически
  }
}

void Client::send_to_server(const std::vector<uint8_t> &data) {
  // Добавляем заголовок длины, как того ждет сервер
  std::vector<uint8_t> packet;
  framer->form_message(data, packet);
  transport->send(socket_visitor->get_fd(), packet);
}

void Client::on_server_message() {
  auto result = transport->receive(socket_visitor->get_fd());
  if (result.status == ReceiveStatus::OK && !result.data.empty()) {
    recv_buffer.insert(recv_buffer.end(), result.data.begin(),
                       result.data.end());

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
bool Client::isConnected() const { return socket_visitor->get_fd() != -1; }
int Client::get_fd() const { return socket_visitor->get_fd(); }
Client::~Client() { disconnect(); }
