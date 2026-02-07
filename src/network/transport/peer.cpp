#include "../../../include/network/transport/peer.hpp"
#include "../../../include/network/transport/raw_socket.hpp"

PeerNodeConnection::PeerNodeConnection(std::unique_ptr<ISocket> socket,
                                       std::unique_ptr<IAcceptor> acceptor,
                                       std::shared_ptr<IEventLoop> event_loop,
                                       const std::string &node_id)
    : listening_socket_(std::move(socket)), acceptor_(std::move(acceptor)),
      event_loop_(event_loop), node_id_(node_id),
      accept_handler_(std::make_shared<PeerAcceptHandler>()) {

  accept_handler_->set_acceptor(std::move(acceptor_));
}

void PeerNodeConnection::start_listening(uint16_t port) {
  listening_socket_->create_socket();
  listening_socket_->make_address_reusable();
  listening_socket_->setup_server(port, "127.0.0.1");

  accept_handler_->init(shared_from_this());
  event_loop_->add_fd(listening_socket_->get_fd(), accept_handler_,
                      EPOLLIN | EPOLLET);
}

bool PeerNodeConnection::connect_to_peer(const std::string &ip_address,
                                         uint16_t port) {
  auto sock = std::make_unique<TCPSocket>();
  if (sock->create_socket() < 0) {
    std::cerr << "Failed to create socket" << std::endl;
    return false;
  }

  int rc = sock->setup_connection(ip_address, port);

  constexpr int MAX_ATTEMPTS = 10;
  int attempts = 0;

  while (rc == 0 && !sock->check_connection_complete(100)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ++attempts;
    if (attempts >= MAX_ATTEMPTS) {
      std::cerr << "Connection timeout to " << ip_address << ":" << port
                << std::endl;
      return false;
    }
  }

  if (rc < 0) {
    std::cerr << "Failed to connect to " << ip_address << ":" << port
              << std::endl;
    return false;
  }

  int fd = sock->get_fd();

  auto peer_conn =
      std::make_shared<PeerConnection>(fd, sock->get_peer_address());
  peer_conn->init_transport(std::make_unique<TCPTransport>());
  register_peer_connection(fd, peer_conn, ip_address);
  std::cout << "Connected to peer " << ip_address << ":" << port << std::endl;
  return true;
}

void PeerNodeConnection::start_event_loop() {
  is_running_.store(true);
  accept_handler_->init(shared_from_this());
  while (is_running_) {
    event_loop_->run_once(100);
  }
}

void PeerNodeConnection::register_peer_connection(
    int fd, std::shared_ptr<IConnection> connection,
    const std::string &ip_address) {
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connection_manager_.add_connection(fd, connection);
    fd_to_peer_id_[fd] = ip_address;
  }

  auto handler =
      std::make_shared<PeerEventHandler>(shared_from_this(), connection);

  event_loop_->add_fd(fd, handler, EPOLLIN | EPOLLOUT | EPOLLET);

  if (state_callback_) {
    state_callback_(ip_address, ConnectionState::CONNECTED);
  }
}

void PeerNodeConnection::disconnect_from_peer(int peer_fd) {
  std::shared_ptr<IConnection> conn;
  std::string peer_id;

  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connection_manager_.get_connection(peer_fd);
    if (!it) {
      std::cerr << "No connection found for fd " << peer_fd << std::endl;
      return;
    }
    conn = *it;
    peer_id = fd_to_peer_id_[peer_fd];

    connection_manager_.remove_connection(peer_fd);
    fd_to_peer_id_.erase(peer_fd);
  }

  if (state_callback_ && !peer_id.empty()) {
    state_callback_(peer_id, ConnectionState::DISCONNECTED);
  }

  std::cout << "Disconnected from peer " << peer_id << std::endl;
}

void PeerNodeConnection::send_to_peer(int peer_fd,
                                      const std::vector<uint8_t> &data) {
  std::shared_ptr<IConnection> conn;
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    conn = *connection_manager_.get_connection(peer_fd);
  }

  if (!conn) {
    std::cerr << "Cannot send, no connection for " << peer_fd << std::endl;
    return;
  }
  conn->queue_send(data);
}

void PeerNodeConnection::broadcast(const std::vector<uint8_t> &data,
                                   BroadcastType type) {
  std::vector<std::shared_ptr<IConnection>> connections;
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto &kv : connection_manager_.get_all_connections()) {
      connections.push_back(kv.second);
    }
  }

  for (auto &conn : connections) {
    conn->queue_send(data);
  }
}

std::vector<int> PeerNodeConnection::get_connected_peers() const {
  std::vector<int> fds;
  {
    for (auto &kv : connection_manager_.get_all_connections()) {
      fds.push_back(kv.first);
    }
  }

  return fds;
}

void PeerNodeConnection::stop_event_loop() { is_running_.store(false); }

ConnectionState PeerNodeConnection::get_connection_state(int peer_fd) {
  std::lock_guard<std::mutex> lock(connections_mutex_);

  auto conn_ptr = connection_manager_.get_connection(peer_fd);
  if (!conn_ptr) {
    return ConnectionState::DISCONNECTED;
  }

  return ConnectionState::CONNECTED;
}

size_t PeerNodeConnection::get_active_connections_count() const {
  std::lock_guard<std::mutex> lock(connections_mutex_);
  return connection_manager_.get_all_connections().size();
}

void PeerNodeConnection::set_message_callback(
    std::function<void(const std::string &, const std::vector<uint8_t> &)>
        callback) {
  message_callback_ = std::move(callback);
}

void PeerNodeConnection::set_state_callback(
    std::function<void(const std::string &, ConnectionState)> callback) {
  state_callback_ = std::move(callback);
}

PeerNodeConnection::~PeerNodeConnection() = default;
