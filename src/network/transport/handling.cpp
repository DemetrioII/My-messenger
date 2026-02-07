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

void PeerAcceptHandler::init(std::shared_ptr<INodeConnection> peer) {
  peer_ = peer;
}

void PeerAcceptHandler::set_acceptor(std::unique_ptr<IAcceptor> acceptor) {
  acceptor_ = std::move(acceptor);
}

void PeerAcceptHandler::handle_event(int fd, uint32_t event_mask) {
  if (event_mask & EPOLLIN) {
    auto client = acceptor_->accept(fd);
    if (client) {
      auto address = (*client)->get_addr();
      char ip_str[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET, &address.sin_addr, ip_str, sizeof(ip_str));
      std::string ip(ip_str);
      auto fd = (*client)->get_fd();
      peer_.lock()->register_peer_connection(fd, *client, ip);
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

PeerEventHandler::PeerEventHandler(std::shared_ptr<INodeConnection> peer_node,
                                   std::shared_ptr<IConnection> connection)
    : peer_node_(peer_node), connection_(connection) {}

void PeerEventHandler::handle_event(int fd, uint32_t event_mask) {}

void ClientHandler::init(std::shared_ptr<IClient> client_) { client = client_; }

void ClientHandler::handle_event(int fd, uint32_t event_mask) {
  if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
    client.lock()->disconnect();
  } else if (event_mask & EPOLLIN) {
    client.lock()->on_server_message();
  }
}

ClientHandler::~ClientHandler() {}
