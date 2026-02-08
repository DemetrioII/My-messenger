#include "../../../include/network/transport/handling.hpp"

void AcceptHandler::init(std::shared_ptr<IServer> server_,
                         std::unique_ptr<IAcceptor> acceptor_) {
  server = server_;
  acceptor = std::move(acceptor_);
}

void AcceptHandler::handle_event(int fd, uint32_t event_mask) {
  if (event_mask & EPOLLIN) {
    auto conn = acceptor->accept(fd);
    if (conn != std::nullopt) {
      server.lock()->on_client_connected(conn.value());
    }
  }
}

void AcceptHandler::set_acceptor(std::unique_ptr<IAcceptor> acceptor_) {
  acceptor = std::move(acceptor_);
}

AcceptHandler::~AcceptHandler() {}

void PeerAcceptHandler::init(std::shared_ptr<INode> peer) { peer_ = peer; }

void PeerAcceptHandler::set_acceptor(std::unique_ptr<IAcceptor> acceptor) {
  acceptor_ = std::move(acceptor);
}

void PeerAcceptHandler::handle_event(int fd, uint32_t event_mask) {
  if (event_mask & EPOLLIN) {
    auto client = acceptor_->accept(fd);
    if (client) {
      peer_.lock()->on_peer_connected(*client);
    }
  }
}

void ServerHandler::init(std::shared_ptr<IServer> server_) { server = server_; }

void ServerHandler::add_client(int fd,
                               std::shared_ptr<IConnection> client_connection) {
  std::lock_guard<std::recursive_mutex> lock(server_mutex);
  clients[fd] = client_connection;
}

void ServerHandler::remove_client(int fd) {
  std::lock_guard<std::recursive_mutex> lock(server_mutex);
  if (clients.find(fd) != clients.end())
    clients.erase(fd);
}

void ServerHandler::handle_event(int fd, uint32_t event_mask) {
  std::lock_guard<std::recursive_mutex> lock(server_mutex);
  if (!server.lock())
    return;

  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
    remove_client(fd);
    server.lock()->on_client_disconnected(fd);
    return;
  }

  if (event_mask & EPOLLIN) {
    auto it = clients.find(fd);
    if (it == clients.end())
      return;

    if (it->second.lock()->try_receive()) {
      if (!it->second.lock()->has_complete_message())
        return;

      auto msg = it->second.lock()->extract_message();
      server.lock()->on_client_message(fd, msg);
    } else {
      remove_client(fd);
      server.lock()->on_client_disconnected(fd);
    }
  }
}

void ServerHandler::clear() { clients.clear(); }

ServerHandler::~ServerHandler() {}

void PeerEventHandler::init(std::shared_ptr<INode> peer_node) {
  peer_node_ = peer_node;
}

void PeerEventHandler::add_peer(int fd,
                                std::shared_ptr<IConnection> peer_connection) {
  std::lock_guard<std::recursive_mutex> lock(peers_mutex_);
  peers_[fd] = peer_connection;
}

void PeerEventHandler::remove_peer(int fd) {
  std::lock_guard<std::recursive_mutex> lock(peers_mutex_);
  if (peers_.find(fd) != peers_.end())
    peers_.erase(fd);
}

void PeerEventHandler::handle_event(int fd, uint32_t event_mask) {
  std::lock_guard<std::recursive_mutex> lock(peers_mutex_);

  if (!peer_node_.lock())
    return;

  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
    remove_peer(fd);
    peer_node_.lock()->on_peer_disconnected(fd);
    return;
  }

  if (event_mask & EPOLLIN) {
    auto it = peers_.find(fd);
    if (it == peers_.end())
      return;

    auto connection = it->second;
    if (!connection) {
      remove_peer(fd);
      peer_node_.lock()->on_peer_disconnected(fd);
      return;
    }

    if (connection->try_receive()) {
      while (connection->has_complete_message()) {
        auto msg = connection->extract_message();
        peer_node_.lock()->on_peer_message(fd, msg);
      }
    } else {
      remove_peer(fd);
      peer_node_.lock()->on_peer_disconnected(fd);
    }
  }

  if (event_mask & EPOLLOUT) {
    peer_node_.lock()->on_peer_writable(fd);
  }
}

void PeerEventHandler::clear() {
  std::lock_guard<std::recursive_mutex> lock(peers_mutex_);
  peers_.clear();
}

PeerEventHandler::~PeerEventHandler() { clear(); }

void ClientHandler::init(std::shared_ptr<IClient> client_) { client = client_; }

void ClientHandler::handle_event(int fd, uint32_t event_mask) {
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
    client.lock()->disconnect();
  } else if (event_mask & EPOLLIN) {
    client.lock()->on_server_message();
  }
}

ClientHandler::~ClientHandler() {}
