#pragma once
#include "connection.hpp"
#include "handling.hpp"
#include "interface.hpp"
#include "raw_socket.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <set>
#include <thread>

enum class ConnectionState { DISCONNECTED, CONNECTED };

class PeerNodeConnection
    : public INodeConnection,
      public std::enable_shared_from_this<PeerNodeConnection> {
private:
  std::unique_ptr<ISocket> listening_socket_;
  std::unique_ptr<IAcceptor> acceptor_;
  ConnectionManager connection_manager_;

  std::shared_ptr<IEventLoop> event_loop_;
  std::shared_ptr<PeerAcceptHandler> accept_handler_;

  std::string node_id_;
  std::atomic<bool> is_running_{false};

  std::unordered_map<int, std::string> fd_to_peer_id_;
  mutable std::mutex connections_mutex_;

  std::set<std::string> bootstrap_nodes_;

  std::function<void(const std::string &, ConnectionState)> state_callback_;

  std::function<void(const std::string &, const std::vector<uint8_t> &)>
      message_callback_;

public:
  PeerNodeConnection(std::unique_ptr<ISocket> socket,
                     std::unique_ptr<IAcceptor> acceptor,
                     std::shared_ptr<IEventLoop> event_loop,
                     const std::string &node_id);

  void register_peer_connection(int fd, std::shared_ptr<IConnection> connection,
                                const std::string &ip) override;

  bool connect_to_peer(const std::string &ip_address, uint16_t port) override;

  void disconnect_from_peer(int peer_fd) override;

  void send_to_peer(int peer_fd, const std::vector<uint8_t> &data) override;

  void broadcast(const std::vector<uint8_t> &data,
                 BroadcastType type = BroadcastType::ALL) override;

  void start_listening(uint16_t port);
  void stop_listening();

  void start_event_loop() override;
  void stop_event_loop() override;

  void on_peer_data(int fd, const std::vector<uint8_t> &data);
  void on_peer_disconnected(int fd);

  ConnectionState get_connection_state(int peer_fd) override;
  std::vector<int> get_connected_peers() const override;
  size_t get_active_connections_count() const override;

  void set_message_callback(
      std::function<void(const std::string &, const std::vector<uint8_t> &)>
          callback) override;

  void set_state_callback(
      std::function<void(const std::string &, ConnectionState)> callback)
      override;

  void add_bootstrap_node(const std::string &ip, uint16_t port);
  void discover_peers();

  std::string get_node_id() const;
  bool is_connected_to_peer(const std::string &peer_id) const;

  ~PeerNodeConnection() override;
};
