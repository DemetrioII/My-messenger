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

struct PeerSession {
  Fd fd;
  FramerMessage framer;
  std::string ip;
  sockaddr_in addr;
  bool handshake_done;
  std::vector<uint8_t> in;
  std::vector<uint8_t> out;
  ITransport transport;

  bool try_receive();

  bool has_complete_message();

  std::vector<uint8_t> extract_message();
};

struct PeerRegistry {
  std::unordered_map<int, std::shared_ptr<PeerSession>> by_fd;
  std::unordered_map<std::string, int> by_ip;
};

struct PeerCallbacks {
  std::function<void(int, std::vector<uint8_t>)> on_data_callback;
  std::function<void(int)> on_peer_connected;
  std::function<void(int)> on_peer_disconnected;
};

struct PeerNode {
  SocketType connecton_type = SocketType::TCP;
  PeerRegistry registry_;
  std::unique_ptr<IEventLoop> event_loop_;
  std::unique_ptr<ISocket> listening_socket_;
  std::unique_ptr<IPeerAcceptor> acceptor_;
  std::shared_ptr<PeerEventHandler> handler_;
  std::shared_ptr<PeerAcceptHandler> accept_handler_;
  PeerCallbacks callbacks_;
  std::atomic<bool> running_{false};

  PeerNode() {}

  PeerNode(std::unique_ptr<ISocket> sock, std::unique_ptr<IPeerAcceptor> acc);

  PeerNode &operator=(PeerNode &&other) {
    registry_ = std::move(other.registry_);
    event_loop_ = std::move(other.event_loop_);
    listening_socket_ = std::move(other.listening_socket_);
    acceptor_ = std::move(other.acceptor_);
    handler_ = std::move(other.handler_);
    callbacks_ = std::move(other.callbacks_);
    accept_handler_ = std::move(other.accept_handler_);
    running_.store(other.running_);
    return *this;
  }

  ~PeerNode();
};

void start_listening(PeerNode &node, int port);

int register_peer_connection(PeerNode &node,
                             std::shared_ptr<PeerSession> session);

int connect_to_peer(PeerNode &node, const std::string &ip, int port);

void send_to_peer(PeerNode &node, int fd, const std::vector<uint8_t> data);

void broadcast(PeerNode &node, const std::vector<uint8_t> &data);

int disconnect_from_peer(PeerNode &node, int fd);

void run_event_loop(PeerNode &node);

void stop(PeerNode &node);

bool flush(PeerSession &session);

void on_peer_writable(PeerNode &node, int fd);

class PeerNodeFactory {
public:
  /**
   * Создание TCP PeerNode
   */
  static std::shared_ptr<PeerNode> create_tcp_peer() {
    auto tcp_socket = std::make_unique<ISocket>(SocketType::TCP);
    std::unique_ptr<IPeerAcceptor> tcp_acceptor =
        std::make_unique<PeerTCPAcceptor>();
    return std::make_shared<PeerNode>(std::move(tcp_socket),
                                      std::move(tcp_acceptor));
  }

  /**
   * Создание UDP PeerNode
   */
  static std::shared_ptr<PeerNode> create_udp_peer() {
    auto udp_socket = std::make_unique<ISocket>(SocketType::UDP);
    std::unique_ptr<IPeerAcceptor> udp_acceptor =
        std::make_unique<PeerUDPAcceptor>();
    return std::make_shared<PeerNode>(std::move(udp_socket),
                                      std::move(udp_acceptor));
  }
};
