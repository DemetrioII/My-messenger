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

  bool flush() override;

  bool try_receive() override;

  // Проверяем, пришло ли сообщение целиком
  bool has_complete_message() const override;

  std::vector<uint8_t> extract_message() override;

  void queue_send(const std::vector<uint8_t> &data) override;

  struct sockaddr_in get_addr();

  int get_fd() const;
  ~ClientConnection() override;
};

class PeerConnection : IConnection {
  int fd;
  bool is_incoming;
  std::string peer_id;
  std::string ip_address;
  uint16_t port;
  ConnectionState state;
  std::unique_ptr<ITransport> transport;
  std::vector<uint8_t> receive_buffer;

public:
  bool flush() override;

  bool try_receive() override;

  bool has_complete_message() const override;

  std::vector<uint8_t> extract_message() override;

  void queue_send(const std::vector<uint8_t> &data) override;

  struct sockaddr_in get_addr() override;

  int get_fd() const override;
  ~PeerConnection();
};

class ConnectionManager {
  std::mutex connections_mutex;
  std::unordered_map<int, std::shared_ptr<IConnection>> connections;

public:
  void add_connection(int fd, std::shared_ptr<IConnection> conn);

  void remove_connection(int fd);

  bool find_connection(int fd);

  void send_to_buffer(int fd, const std::vector<uint8_t> &data);

  std::optional<std::shared_ptr<IConnection>> get_connection(int fd);

  ~ConnectionManager();
};
