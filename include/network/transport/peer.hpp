#pragma once
#include "connection.hpp"
#include "handling.hpp"
#include "interface.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <set>

enum class ConnectionState { DISCONNECTED, CONNECTED };

class PeerNodeConnection
    : public INodeConnection,
      public std::enable_shared_from_this<PeerNodeConnection> {
  std::unique_ptr<ISocket> socket_;
  std::unique_ptr<ITransport> transport_;
  std::unique_ptr<IEventLoop> event_loop_;

  std::function<void(const std::string &, const std::vector<uint8_t> &)>
      on_message_callback;
  std::function<void(const std::string &, ConnectionState)> on_state_callback;
  std::function<void(const std::string &)> on_peer_callback;
  std::function<void(const std::string &)> on_peer_disconnected_callback;

  std::unordered_map<std::string, std::shared_ptr<PeerConnection>>
      peer_connections;
  std::unordered_map<int, std::string> fd_to_peer_id;
  std::mutex connections_mutex;

  std::unique_ptr<ISocket> listening_socket_;
  std::string node_id_;
  uint16_t listening_port;
  std::atomic<bool> is_listening{false};
  std::atomic<bool> is_running{false};

  std::set<std::string> bootstrap_nodes;
  std::atomic<uint32_t> max_connections{256};
  std::atomic<uint32_t> connection_timeout_ms{30000};

  void handle_incoming_connection(int fd, const std::string &ip, uint16_t port);
  void handle_peer_data(int fd, const std::vector<uint8_t> &data);
  void handle_peer_disconnected(int fd);
  void cleanup_dead_connections();
  bool validate_peer_id(const std::string &peer_id) const;

  PeerNodeConnection(std::unique_ptr<ISocket> socket,
                     std::unique_ptr<IEventLoop> event_loop,
                     const std::string &node_id)
      : socket_(std::move(socket)), event_loop_(std::move(event_loop)),
        node_id_(node_id) {}

public:
  ~PeerNodeConnection() override;

  static std::shared_ptr<PeerNodeConnection>
  create(std::unique_ptr<ISocket> socket,
         std::shared_ptr<IEventLoop> event_loop, const std::string &node_id);

  bool connect_to_peer(int peer_fd, const std::string &peer_id,
                       uint16_t port) override;

  void disconnect_from_peer(int peer_fd) override;

  void send_to_peer(int peer_fd, const std::vector<uint8_t> &data) override;

  void broadcast(const std::vector<uint8_t> &data,
                 BroadcastType type = BroadcastType::ALL) override;

  void start_listening(uint16_t port);
  void stop_listening();

  void start_event_loop();
  void stop_event_loop();

  ConnectionState get_connection_state(int peer_fd) const override;
  std::vector<int> get_connected_peers() const override;
  size_t get_active_connections_count() const override;

  bool has_peer(const std::string &peer_id) const;
  void close_all_connections();

  void add_bootstrap_node(const std::string &ip, uint16_t port);
  void remove_bootstrap_node(const std::string &ip, uint16_t port);
  void discover_peers();
  void ping_peer(int fd);

  void set_message_callback(
      std::function<void(const std::string &, const std::vector<uint8_t> &)>
          callback) override;

  void set_state_callback(
      std::function<void(const std::string &, ConnectionState)> callback)
      override;

  void
  set_peer_connected_callbck(std::function<void(const std::string &)> callback);

  void set_peer_disconnected_callback(
      std::function<void(const std::string &)> callback);

  void set_max_connections(uint32_t max);
  void set_connection_timeout(uint32_t timeout_ms);

  std::string get_node_id() const;
  uint16_t get_listening_port() const;
  bool is_connected_to_peer(const std::string &peer_id) const;

  void on_socket_readable(int fd, const std::vector<uint8_t> &data);
  void on_socket_writable(int fd);
  void on_socket_error(int fd, int error);
};
