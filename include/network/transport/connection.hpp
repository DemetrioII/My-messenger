#pragma once
#include "../protocol/framing.hpp"
#include "interface.hpp"
#include <cstring>
#include <mutex>
#include <optional>
#include <unistd.h>
#include <unordered_map>
#include <vector>

class ClientConnection : public IConnection {
  Fd fd;
  std::unique_ptr<FramerMessage> framer;
  struct sockaddr_in addr;
  std::unique_ptr<ITransport> transport;
  std::vector<uint8_t> recv_buffer;
  std::vector<uint8_t> send_buffer;

public:
  ClientConnection(int fd_);
  explicit ClientConnection(int fd_, const struct sockaddr_in &addr_);

  void init_transport(std::unique_ptr<ITransport> transport_);

  ClientConnection(const ClientConnection &) = delete;
  ClientConnection &operator=(const ClientConnection &) = delete;

  // ЧИНЯЕМ MOVE: сбрасываем fd у донора
  ClientConnection(ClientConnection &&other) noexcept;

  ClientConnection &operator=(ClientConnection &&other) noexcept;

  bool flush();

  bool try_receive();

  // Проверяем, пришло ли сообщение целиком
  bool has_complete_message() const;

  std::vector<uint8_t> extract_message();

  void queue_send(const std::vector<uint8_t> &data);

  struct sockaddr_in get_addr();

  int get_fd() const;
  ~ClientConnection() override;
};

class ConnectionManager {
  std::mutex connections_mutex;
  std::unordered_map<int, std::shared_ptr<ClientConnection>> connections;

public:
  void add_connection(int fd, std::shared_ptr<ClientConnection> conn);

  void remove_connection(int fd);

  bool find_connection(int fd);

  void send_to_buffer(int fd, const std::vector<uint8_t> &data);

  std::optional<std::shared_ptr<ClientConnection>> get_connection(int fd);

  ~ConnectionManager();
};
