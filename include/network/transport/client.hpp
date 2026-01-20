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

class TCPClient : public IClient,
                  public std::enable_shared_from_this<TCPClient> {
  TransportFabric transport_fabric;
  std::shared_ptr<ISocket> socket_visitor;
  std::unique_ptr<EventLoop> event_loop;
  std::shared_ptr<ClientHandler> handler;
  std::unique_ptr<ITransport> transport;
  std::unique_ptr<FramerMessage> framer;
  std::function<void(const std::vector<uint8_t> &)> on_data_callback;
  std::vector<uint8_t> recv_buffer; // Буфер для склейки сообщений

  TCPClient()
      : event_loop(std::make_unique<EventLoop>()),
        handler(std::make_shared<ClientHandler>()),
        socket_visitor(std::make_unique<TCPSocket>()) {}

public:
  static std::shared_ptr<TCPClient> create() {
    struct make_shared_enabler : public TCPClient {};
    return std::make_shared<make_shared_enabler>();
  }

  void set_data_callback(
      std::function<void(const std::vector<uint8_t> &)> f) override {
    on_data_callback = f;
  }

  bool connect(const std::string &server_ip, int port) override {
    handler->init(shared_from_this());
    if (socket_visitor->create_socket() == -1)
      return false;
    if (socket_visitor->setup_connection(server_ip, port) < 0)
      return false;
    transport = std::move(transport_fabric.create_tcp());
    transport->connect(socket_visitor->get_fd());
    framer = std::make_unique<FramerMessage>();
    event_loop->add_fd(socket_visitor->get_fd(), handler, EPOLLIN | EPOLLRDHUP);
    return true;
  }

  void disconnect() override {
    if (socket_visitor->get_fd() != -1) {
      event_loop->remove_fd(socket_visitor->get_fd());
      ::shutdown(socket_visitor->get_fd(), SHUT_RDWR);
      socket_visitor->close(); // Вызовется в деструкторе Fd автоматически
    }
  }

  void send_to_server(const std::vector<uint8_t> &data) override {
    // Добавляем заголовок длины, как того ждет сервер
    std::vector<uint8_t> packet;
    framer->form_message(data, packet);
    transport->send(socket_visitor->get_fd(), packet);
  }

  void on_server_message() override {
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

  void run_event_loop() override {
    std::cout << "Введите сообщение (или /exit): " << std::endl;
    while (isConnected()) {
      event_loop->run_once(50); // Небольшой таймаут, чтобы не грузить CPU
      // Внимание: std::cin блокирует поток!
      // Для тестов это ок, но сообщения от сервера будут видны только ПОСЛЕ
      // ввода
    }
  }

  void on_writable(int fd) override {}
  void on_disconnected() override {
    std::cout << "Соединение разорвано" << std::endl;
  }
  bool isConnected() const { return socket_visitor->get_fd() != -1; }
  int get_fd() const { return socket_visitor->get_fd(); }
  ~TCPClient() override { disconnect(); }
};
