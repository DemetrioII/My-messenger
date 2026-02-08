#include "../../../include/network/transport/peer.hpp"

PeerNode::PeerNode(std::unique_ptr<ISocket> listening_socket,
                   std::unique_ptr<IAcceptor> acceptor)
    : listening_socket_(std::move(listening_socket)),
      acceptor_(std::move(acceptor)),
      event_loop_(std::make_unique<EventLoop>()),
      handler_(std::make_shared<PeerEventHandler>()),
      accept_handler_(std::make_shared<PeerAcceptHandler>()) {

  accept_handler_->set_acceptor(std::move(acceptor_));
}

void PeerNode::start_listening(int port) {
  listening_socket_->create_socket();
  if (listening_socket_->get_fd() != -1) {
    is_running_ = true;
    listening_socket_->setup_server(port, "0.0.0.0");
    listening_socket_->make_address_reusable();

    accept_handler_->init(shared_from_this());
    handler_->init(shared_from_this());

    event_loop_->add_fd(listening_socket_->get_fd(), accept_handler_,
                        EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP);

    std::cout << "PeerNode listening on port " << port << std::endl;
  } else {
    throw std::runtime_error("Failed to create listening socket");
  }
}

void PeerNode::register_peer_connection(int fd,
                                        std::shared_ptr<IConnection> connection,
                                        const std::string &ip) {
  std::cout << "New peer: " << ip << " (fd=" << fd << ")" << std::endl;

  connection->init_transport(TransportFabric::create_tcp());

  connection_manager_.add_connection(fd, connection);

  handler_->add_peer(fd, connection);

  {
    std::lock_guard<std::mutex> lock(peer_ips_mutex);
    peer_ips[fd] = ip;
  }

  event_loop_->add_fd(fd, handler_,
                      EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP);

  if (on_peer_connected_callback)
    on_peer_connected_callback(ip);
}

void PeerNode::on_peer_connected(std::shared_ptr<IConnection> connection) {

  auto address = connection->get_addr();
  char ip_str[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET, &address.sin_addr, ip_str, sizeof(ip_str));
  std::string ip(ip_str);
  auto fd = connection->get_fd();
  register_peer_connection(connection->get_fd(), connection, ip);
}

bool PeerNode::connect_to_peer(const std::string &peer_ip, int port) {
  std::unique_ptr<ISocket> peer_socket; // Умирает при выходе
  if (listening_socket_->get_type() == SocketType::TCP) {
    peer_socket = std::make_unique<TCPSocket>();
  } else {
    peer_socket = std::make_unique<UDPSocket>();
  }

  if (peer_socket->create_socket() == -1) {
    std::cerr << "Failed to create socket for peer connection" << std::endl;
    return false;
  }

  if (peer_socket->setup_connection(peer_ip, port) < 0) {
    std::cerr << "Failed to connect to peer " << peer_ip << ":" << port
              << std::endl;
    return false;
  }

  sockaddr_in peer_addr = peer_socket->get_peer_address();
  int fd = peer_socket->get_fd();

  peer_sockets[fd] = std::move(peer_socket);

  auto connection = std::make_shared<PeerConnection>(fd, peer_addr);

  register_peer_connection(fd, connection, peer_ip);

  std::cout << "Connected to peer " << peer_ip << ":" << port << " (fd=" << fd
            << ")" << std::endl;

  return true;
}

void PeerNode::send_to_peer(const std::string &peer_ip,
                            const std::vector<uint8_t> &data) {
  int fd = get_peer_fd(peer_ip);
  if (fd != -1) {
    send_to_peer_by_fd(fd, data);
  } else {
    std::cerr << "Peer not found: " << peer_ip << std::endl;
  }
}

void PeerNode::send_to_peer_by_fd(int peer_fd,
                                  const std::vector<uint8_t> &data) {
  connection_manager_.send_to_buffer(peer_fd, data);
  event_loop_->enable_write(peer_fd);
}

void PeerNode::broadcast(const std::vector<uint8_t> &data) {
  auto all_connections = connection_manager_.get_all_connections();

  for (const auto &[fd, connection] : all_connections) {
    connection_manager_.send_to_buffer(fd, data);
    event_loop_->enable_write(fd);
  }

  std::cout << "Broadcast message to " << all_connections.size() << " peers"
            << std::endl;
}

void PeerNode::disconnect_from_peer(int peer_fd) {
  std::cout << "Disconnecting peer (fd=" << peer_fd << ")" << std::endl;

  if (event_loop_)
    event_loop_->remove_fd(peer_fd);

  handler_->remove_peer(peer_fd);

  connection_manager_.remove_connection(peer_fd);

  {
    std::lock_guard<std::mutex> lock(peer_ips_mutex);
    peer_ips.erase(peer_fd);
  }

  peer_sockets.erase(peer_fd);
}

void PeerNode::disconnect_from_peer_by_ip(const std::string &peer_ip) {
  int fd = get_peer_fd(peer_ip);

  if (fd != -1) {
    disconnect_from_peer(fd);
  }
}

void PeerNode::run_event_loop() {
  while (is_running()) {
    event_loop_->run_once(100);
  }
}

void PeerNode::stop() {
  std::cout << "Stopping PeerNode..." << std::endl;

  if (!is_running_) {
    return;
  }

  is_running_ = false;
  if (event_loop_)
    event_loop_->stop();

  auto all_connections = connection_manager_.get_all_connections();
  for (const auto &[fd, connection] : all_connections) {
    disconnect_from_peer(fd);
  }

  if (listening_socket_)
    listening_socket_->close();

  if (handler_)
    handler_->clear();
}

bool PeerNode::is_running() const { return is_running_; }

void PeerNode::set_data_callback(
    std::function<void(const std::string &ip, const std::vector<uint8_t> &data)>
        callback) {
  on_data_callback = callback;
}

void PeerNode::set_peer_connected_callback(
    std::function<void(const std::string &ip)> callback) {
  on_peer_connected_callback = callback;
}

void PeerNode::set_peer_disconnected_callback(
    std::function<void(const std::string &ip)> callback) {
  on_peer_disconnected_callback = callback;
}

std::vector<std::string> PeerNode::get_connected_peers() const {
  std::lock_guard<std::mutex> lock(peer_ips_mutex);

  std::vector<std::string> peers;
  peers.reserve(peer_ips.size());

  for (const auto &[fd, ip] : peer_ips) {
    peers.push_back(ip);
  }

  return peers;
}

size_t PeerNode::get_active_connections_count() const {
  return connection_manager_.get_all_connections().size();
}

std::string PeerNode::get_peer_ip(int fd) const {
  std::lock_guard<std::mutex> lock(peer_ips_mutex);

  auto it = peer_ips.find(fd);
  if (it != peer_ips.end()) {
    return it->second;
  }

  return "";
}

int PeerNode::get_peer_fd(const std::string &ip) const {
  std::lock_guard<std::mutex> lock(peer_ips_mutex);

  for (const auto &[fd, peer_ip] : peer_ips) {
    if (peer_ip == ip)
      return fd;
  }

  return -1;
}

void PeerNode::on_peer_writable(int fd) {
  auto done = (*connection_manager_.get_connection(fd))->flush();
  if (done)
    event_loop_->disable_write(fd);
}

void PeerNode::on_peer_message(int fd, const std::vector<uint8_t> &data) {
  if (!data.empty()) {
    std::string peer_ip = get_peer_ip(fd);

    if (on_data_callback)
      on_data_callback(peer_ip, data);
  }
}

void PeerNode::on_peer_disconnected(int fd) {
  std::string peer_ip = get_peer_ip(fd);
  std::cout << "Peer disconnected: " << peer_ip << " (fd=" << fd << ")"
            << std::endl;

  if (on_peer_disconnected_callback && !peer_ip.empty())
    on_peer_disconnected_callback(peer_ip);

  disconnect_from_peer(fd);
}

void PeerNode::on_peer_error(int fd) {
  std::cerr << "Peer error on fd=" << fd << std::endl;
  on_peer_disconnected(fd);
}

PeerNode::~PeerNode() {
  std::cout << "~PeerNode()" << std::endl;
  stop();
}
