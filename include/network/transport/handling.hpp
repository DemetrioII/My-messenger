// handling.hpp
#pragma once
#include "acceptor.hpp"
#include "interface.hpp"
#include <mutex>
#include <unordered_map>

class Acceptor;

class AcceptHandler : public IEventHandler {
  std::weak_ptr<IServer> server;
  Acceptor acceptor;

public:
  AcceptHandler() = default;
  void init(std::shared_ptr<IServer> server_, Acceptor acceptor_) {
    server = server_;
    acceptor = acceptor_;
  }

  void handle_event(int fd, uint32_t event_mask) override {
    if (event_mask & EPOLLIN) {
      auto conn = acceptor.accept(fd);
      if (conn != std::nullopt) {
        server.lock()->on_client_connected(conn.value());
      }
    }
  }

  ~AcceptHandler() override {}
};

class ServerHandler : public IEventHandler {
  std::weak_ptr<IServer> server;
  std::recursive_mutex server_mutex;
  // std::unordered_map<int, std::shared_ptr<T>> transport;
  std::unordered_map<int, std::weak_ptr<ClientConnection>> clients;

public:
  ServerHandler() = default;
  void init(std::shared_ptr<IServer> server_) { server = server_; }

  void add_client(int fd, std::shared_ptr<ClientConnection> client_connection) {
    std::lock_guard<std::recursive_mutex> lock(server_mutex);
    clients[fd] = client_connection;
  }

  void remove_client(int fd) {
    std::lock_guard<std::recursive_mutex> lock(server_mutex);
    if (clients.find(fd) != clients.end())
      clients.erase(fd);
  }

  void handle_event(int fd, uint32_t event_mask) override {
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

  void clear() { clients.clear(); }

  ~ServerHandler() override {}
};

class ClientHandler : public IEventHandler {
  std::weak_ptr<IClient> client;

public:
  ClientHandler() = default;
  void init(std::shared_ptr<IClient> client_) { client = client_; }

  void handle_event(int fd, uint32_t event_mask) override {
    if (event_mask & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
      client.lock()->disconnect();
    } else if (event_mask & EPOLLIN) {
      client.lock()->on_server_message();
    }
  }

  ~ClientHandler() override {}
};
