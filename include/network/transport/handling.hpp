// handling.hpp
#pragma once
#include "acceptor.hpp"
#include "interface.hpp"
#include <mutex>
#include <unordered_map>

class TCPAcceptor;

class AcceptHandler : public IEventHandler {
  std::weak_ptr<IServer> server;
  std::unique_ptr<IAcceptor> acceptor;

public:
  AcceptHandler() = default;

  void init(std::shared_ptr<IServer> server_,
            std::unique_ptr<IAcceptor> acceptor_);

  void set_acceptor(std::unique_ptr<IAcceptor> acceptor);

  void handle_event(int fd, uint32_t event_mask) override;

  ~AcceptHandler() override;
};

class ServerHandler : public IEventHandler {
  std::weak_ptr<IServer> server;
  std::recursive_mutex server_mutex;
  // std::unordered_map<int, std::shared_ptr<T>> transport;
  std::unordered_map<int, std::weak_ptr<ClientConnection>> clients;

public:
  ServerHandler() = default;

  void init(std::shared_ptr<IServer> server_);

  void add_client(int fd, std::shared_ptr<ClientConnection> client_connection);

  void remove_client(int fd);

  void handle_event(int fd, uint32_t event_mask) override;

  void clear();

  ~ServerHandler() override;
};

class ClientHandler : public IEventHandler {
  std::weak_ptr<IClient> client;

public:
  ClientHandler() = default;

  void init(std::shared_ptr<IClient> client_);

  void handle_event(int fd, uint32_t event_mask) override;

  ~ClientHandler() override;
};
