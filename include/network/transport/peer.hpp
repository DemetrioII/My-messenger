#pragma once
#include "connection.hpp"
#include "event_loop.hpp"
#include "handling.hpp"
#include "interface.hpp"
#include "raw_socket.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <set>
#include <thread>

enum class ConnectionState { DISCONNECTED, CONNECTED };

class PeerNode : public INode, public std::enable_shared_from_this<PeerNode> {
private:
  std::unique_ptr<ISocket> listening_socket_;
  std::unique_ptr<IAcceptor> acceptor_;
  ConnectionManager connection_manager_;

  std::unique_ptr<IEventLoop> event_loop_;
  std::shared_ptr<PeerAcceptHandler> accept_handler_;
  std::shared_ptr<PeerEventHandler> handler_;

  std::unordered_map<int, std::unique_ptr<ISocket>> peer_sockets;

  std::atomic<bool> is_running_{false};

  std::unordered_map<int, std::string> peer_ips;
  mutable std::mutex peer_ips_mutex;

  std::function<void(const std::string &ip, const std::vector<uint8_t> &data)>
      on_data_callback;
  std::function<void(const std::string &ip)> on_peer_connected_callback;
  std::function<void(const std::string &ip)> on_peer_disconnected_callback;

  PeerNode(std::unique_ptr<ISocket> listening_socket,
           std::unique_ptr<IAcceptor> acceptor);

public:
  static std::shared_ptr<PeerNode>
  create(std::unique_ptr<ISocket> listening_socket,
         std::unique_ptr<IAcceptor> acceptor) {
    struct make_shared_enabler : public PeerNode {
      make_shared_enabler(std::unique_ptr<ISocket> sock,
                          std::unique_ptr<IAcceptor> acc)
          : PeerNode(std::move(sock), std::move(acc)) {}
    };
    return std::make_shared<make_shared_enabler>(std::move(listening_socket),
                                                 std::move(acceptor));
  }

  PeerNode(const PeerNode &) = delete;
  PeerNode &operator=(const PeerNode &) = delete;

  void start_listening(int port) override;

  void register_peer_connection(int fd, std::shared_ptr<IConnection> connection,
                                const std::string &ip) override;

  bool connect_to_peer(const std::string &peer_ip, int port) override;

  void send_to_peer(const std::string &peer_ip,
                    const std::vector<uint8_t> &data) override;

  void send_to_peer_by_fd(int peer_fd,
                          const std::vector<uint8_t> &data) override;

  void broadcast(const std::vector<uint8_t> &data) override;

  void disconnect_from_peer(int peer_fd) override;

  void disconnect_from_peer_by_ip(const std::string &peer_ip) override;

  void run_event_loop() override;

  void stop() override;

  bool is_running() const override;

  void set_data_callback(std::function<void(const std::string &ip,
                                            const std::vector<uint8_t> &data)>
                             callback) override;

  void set_peer_connected_callback(
      std::function<void(const std::string &ip)> callback) override;

  void set_peer_disconnected_callback(
      std::function<void(const std::string &ip)> callback) override;

  std::vector<std::string> get_connected_peers() const override;

  size_t get_active_connections_count() const override;

  std::string get_peer_ip(int fd) const override;

  int get_peer_fd(const std::string &ip) const override;

  void on_peer_connected(std::shared_ptr<IConnection> connection) override;

  void on_peer_message(int fd, const std::vector<uint8_t> &data) override;

  void on_peer_disconnected(int fd) override;

  void on_peer_error(int fd) override;

  void on_peer_writable(int fd) override;

  ~PeerNode() override;
};

class PeerNodeFabric {
public:
  /**
   * Создание TCP PeerNode
   */
  static std::shared_ptr<PeerNode> create_tcp_peer() {}

  /**
   * Создание UDP PeerNode
   */
  static std::shared_ptr<PeerNode> create_udp_peer() {}
};
