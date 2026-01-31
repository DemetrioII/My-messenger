// interface.hpp
#pragma once
#include <arpa/inet.h>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <stdint.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <variant>
#include <vector>

#define MAX_EVENTS 10

enum class SocketType { TCP, UDP };
class ClientConnection;

class Fd {
  int fd;

public:
  Fd() : fd(-1) {}
  Fd(int fd_) : fd(fd_) {}
  Fd(const Fd &) = delete;
  Fd &operator=(const Fd &) = delete;
  Fd(Fd &&other) noexcept : fd(other.fd) { other.fd = -1; }
  Fd &operator=(Fd &&other) noexcept {
    if (this != &other) {
      reset_fd(-1);
      fd = other.fd;
      other.fd = -1;
    }
    return *this;
  }
  int get_fd() const { return fd; }

  void reset_fd(int new_fd) {
    if (fd != -1)
      ::close(fd);
    fd = new_fd;
  }

  ~Fd() { reset_fd(-1); }
};

struct NetBuffer {
  static constexpr size_t SBO_SIZE = 128;
  size_t length = 0;
  bool is_large = false;

  std::variant<std::array<uint8_t, SBO_SIZE>, std::vector<uint8_t>> storage;

  NetBuffer() : storage(std::array<uint8_t, SBO_SIZE>{}), length(0) {}

  void set_data(const uint8_t *src, size_t len) {
    length = len;
    if (len <= SBO_SIZE) {
      auto &arr = std::get<std::array<uint8_t, SBO_SIZE>>(storage);
      std::memcpy(arr.data(), src, len);
    } else {
      storage.emplace<std::vector<uint8_t>>(src, src + len);
    }
  }

  const uint8_t *data() const {
    if (auto *arr = std::get_if<std::array<uint8_t, SBO_SIZE>>(&storage)) {
      return arr->data();
    }
    return std::get<std::vector<uint8_t>>(storage).data();
  }
};

enum class ReceiveStatus { OK, WOULDBLOCK, CLOSED, ERROR };

struct ReceiveResult {
  ReceiveStatus status;
  NetBuffer data;
  int error_code;
};

class IAcceptor {
public:
  virtual std::optional<std::shared_ptr<ClientConnection>>
  accept(int server_fd) = 0;
  virtual ~IAcceptor() = default;
};

class ITransport {
public:
  virtual ssize_t send(int fd, const std::vector<uint8_t> &data) const = 0;
  virtual ReceiveResult receive(int fd) const = 0;
  virtual void connect(int fd) = 0;
  virtual ~ITransport() = default;
};

class ISocket {
public:
  virtual int create_socket() = 0;
  virtual void setup_server(uint16_t PORT, const std::string &ip) = 0;
  virtual int get_fd() const = 0;
  virtual void close() = 0;
  virtual void make_address_reusable() = 0;
  virtual int setup_connection(const std::string &ip, uint16_t port) = 0;
  virtual sockaddr_in get_peer_address() const = 0;
  virtual SocketType get_type() const = 0;
  virtual ~ISocket() = default;
};

class IServer {
public:
  virtual void start(int port) = 0;
  virtual void stop() = 0;
  virtual bool is_running() const = 0;
  virtual void on_client_error(int fd) = 0;
  virtual void on_client_connected(std::shared_ptr<ClientConnection> conn) = 0;
  virtual void on_client_disconnected(int fd) = 0;
  virtual void on_client_message(int fd, const std::vector<uint8_t> &data) = 0;
  virtual void on_client_writable(int fd) = 0;
  virtual void send(int fd, const std::vector<uint8_t> &data) = 0;
  virtual void set_data_callback(
      std::function<void(int, std::vector<uint8_t>)> callback) = 0;
  virtual void run_event_loop() = 0;
  virtual ~IServer() = default;
};

class IClient {
public:
  virtual void on_writable(int fd) = 0;
  virtual void on_disconnected() = 0;
  virtual void run_event_loop() = 0;
  virtual void init_transport(std::unique_ptr<ITransport> transport_) = 0;
  virtual void
  set_data_callback(std::function<void(const std::vector<uint8_t> &)> f) = 0;
  virtual void send_to_server(const std::vector<uint8_t> &data) = 0;
  virtual void on_server_message() = 0;
  virtual bool connect(const std::string &server_ip, int port) = 0;
  virtual void disconnect() = 0;
  virtual ~IClient() = default;
};

class INodeConnection;

class INode {
public:
  virtual bool connect_to_peer(const std::string &ip, uint16_t port) = 0;
  virtual void disconnect_peer(std::shared_ptr<INodeConnection> peer) = 0;
  virtual void send_to_peer(std::shared_ptr<INodeConnection> peer,
                            const std::vector<uint8_t> &data) = 0;
  virtual void broadcast(const std::vector<uint8_t> &data) = 0;
  virtual ~INode() = default;
};

class INodeConnection {
public:
  virtual int get_fd() const = 0;
  virtual std::string get_peer_address() const = 0;
  virtual void send(const std::vector<uint8_t> &data) = 0;
  virtual void close() = 0;
  virtual ~INodeConnection() = default;
};

class IEventHandler {
public:
  virtual void handle_event(int fd, uint32_t events) = 0;
  virtual ~IEventHandler() = default;
};

class IEventLoop {
public:
  virtual void add_fd(int fd, std::weak_ptr<IEventHandler> handler,
                      uint32_t events) = 0;
  virtual void run_once(int timeout_ms = -1) = 0;
  virtual void stop() = 0;
  virtual ~IEventLoop() = default;
};

class IConnection {
public:
  virtual ~IConnection() = default;
};
