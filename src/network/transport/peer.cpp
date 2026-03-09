#include "../../../include/network/transport/peer.hpp"

PeerNode::PeerNode(std::unique_ptr<ISocket> listening_socket,
                   std::unique_ptr<IPeerAcceptor> acceptor)
    : listening_socket_(std::move(listening_socket)),
      acceptor_(std::move(acceptor)),
      event_loop_(std::make_unique<EventLoop>()),
      handler_(std::make_shared<PeerEventHandler>()),
      accept_handler_(std::make_shared<PeerAcceptHandler>()) {

  accept_handler_->set_acceptor(std::move(acceptor_));
}

void start_listening(PeerNode &node, int port) {
  node.listening_socket_->setup_server("0.0.0.0", port);
  if (node.listening_socket_->fd != -1) {
    node.running_ = true;

    node.accept_handler_->init(&node);
    node.handler_->init(&node);

    node.event_loop_->add_fd(node.listening_socket_->fd, node.accept_handler_,
                             EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP);

    std::cout << "PeerNode listening on port " << port << std::endl;
  } else {
    throw std::runtime_error("Failed to create listening socket");
  }
}

int register_peer_connection(PeerNode &node,
                             std::shared_ptr<PeerSession> session) {
  std::cout << "New peer: " << session->ip << " (fd=" << session->fd.get_fd()
            << ")" << std::endl;

  if (node.registry_.by_fd.find(session->fd.get_fd()) !=
      node.registry_.by_fd.end())
    return 1; // Peer is already connected

  node.handler_->add_peer(session);

  node.event_loop_->add_fd(session->fd.get_fd(), node.handler_,
                           EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP);

  if (node.callbacks_.on_peer_connected) {
    node.callbacks_.on_peer_connected(session->fd.get_fd());
  }

  switch (node.connecton_type) {
  case SocketType::TCP: {
    // session.transport = TransportFactory::create_tcp(session.fd);
    node.registry_.by_ip[session->ip] = session->fd.get_fd();
    node.registry_.by_fd[session->fd.get_fd()] = session;
    break;
  }
  case SocketType::UDP: {
    // session.transport = TransportFactory::create_udp(session.fd,
    // session.addr);
    node.registry_.by_ip[session->ip] = session->fd.get_fd();
    node.registry_.by_fd[session->fd.get_fd()] = session;
    break;
  }
  }
  return 0;
}

bool PeerSession::try_receive() {
  auto result = receive(transport);
  if (result.data.data()) {
    in.insert(in.end(), result.data.data(),
              result.data.data() + result.data.length);
  }
  return (result.status == ReceiveStatus::OK ||
          result.status == ReceiveStatus::WOULDBLOCK);
}

bool PeerSession::has_complete_message() {
  return framer.has_message_in_buffer(in);
}

std::vector<uint8_t> PeerSession::extract_message() {
  return framer.extract_message(in);
}

int connect_to_peer(PeerNode &node, const std::string &peer_ip, int port) {
  std::unique_ptr<ISocket> peer_socket; // Умирает при выходе
  if (node.listening_socket_->type == SocketType::TCP) {
    peer_socket = std::make_unique<ISocket>(SocketType::TCP);
  } else {
    peer_socket = std::make_unique<ISocket>(SocketType::UDP);
  }

  if (peer_socket->setup_connection(peer_ip, port) < 0) {
    std::cerr << "Failed to connect to peer " << peer_ip << ":" << port
              << std::endl;
    return 0;
  }

  sockaddr_in peer_addr = peer_socket->addr;
  int fd = peer_socket->fd;

  register_peer_connection(
      node, std::make_shared<PeerSession>(
                PeerSession{.fd = peer_socket->fd,
                            .ip = peer_ip,
                            .addr = peer_socket->addr,
                            .transport = TransportFactory::create_tcp(fd)}));

  std::cout << "Connected to peer " << peer_ip << ":" << port << " (fd=" << fd
            << ")" << std::endl;

  return fd;
}

void send_to_buffer(PeerSession &session, const std::vector<uint8_t> &data) {
  session.framer.form_message(data, session.out);
}

void send_to_peer(PeerNode &node, int fd, const std::vector<uint8_t> data) {
  if (node.registry_.by_fd.find(fd) != node.registry_.by_fd.end()) {
    send_to_buffer(*node.registry_.by_fd[fd], data);
    node.event_loop_->enable_write(fd);
  } else {
    std::cerr << "Peer not found: " << fd << std::endl;
  }
}

void broadcast(PeerNode &node, const std::vector<uint8_t> &data) {
  for (auto &[fd, session] : node.registry_.by_fd) {
    send_to_buffer(*session, data);
    node.event_loop_->enable_write(fd);
  }

  std::cout << "Broadcast message to " << node.registry_.by_ip.size()
            << " peers" << std::endl;
}

int disconnect_from_peer(PeerNode &node, int fd) {
  std::string peer_ip = node.registry_.by_fd[fd]->ip;
  if (node.registry_.by_ip.find(peer_ip) == node.registry_.by_ip.end())
    return 1;

  std::cout << "Disconnecting from " << peer_ip << std::endl;

  node.event_loop_->remove_fd(fd);
  node.handler_->remove_peer(fd);

  node.registry_.by_fd.erase(fd);
  node.registry_.by_ip.erase(peer_ip);

  if (node.callbacks_.on_peer_disconnected)
    node.callbacks_.on_peer_disconnected(fd);

  return 0;
}

void run_event_loop(PeerNode &node) {
  while (node.running_) {
    node.event_loop_->run_once(100);
  }
}

void stop(PeerNode &node) {
  std::cout << "Stopping PeerNode..." << std::endl;

  if (!node.running_) {
    return;
  }

  std::vector<int> peer_fds;
  peer_fds.reserve(node.registry_.by_ip.size());
  for (const auto &[fd, _] : node.registry_.by_fd)
    peer_fds.push_back(fd);

  for (const auto &fd : peer_fds) {
    disconnect_from_peer(node, fd);
  }

  node.running_ = false;
  if (node.event_loop_)
    node.event_loop_->stop();

  if (node.handler_)
    node.handler_->clear();
}

bool flush(PeerSession &session) {
  if (session.out.empty())
    return true;
  ssize_t sent = send(session.transport, session.out);
  if (sent > 0) {
    session.out.erase(session.out.begin(), session.out.begin() + sent);
    return session.out.empty();
  }
  return (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK));
}

void on_peer_writable(PeerNode &node, int fd) {
  auto done = flush(*node.registry_.by_fd[fd]);
  if (done)
    node.event_loop_->disable_write(fd);
}

PeerNode::~PeerNode() {
  std::cout << "~PeerNode()" << std::endl;
  stop(*this);
}
