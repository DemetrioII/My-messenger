#pragma once
#include "../network/protocol/parser.hpp"
#include "../network/transport/interface.hpp"
#include "../network/transport/server.hpp"
#include "encryption_service.hpp"
#include "message_queue.hpp"
#include "messageing_service.hpp"
#include <fstream>
#include <functional>

struct ClientContext {
  std::function<void(const std::string &)> ui_callback = nullptr;
  std::shared_ptr<IClient> client;
  std::shared_ptr<MessageQueue> mq;

  std::shared_ptr<EncryptionService> encryption_service;
  Parser parser;
  std::shared_ptr<Serializer> serializer;
  std::vector<uint8_t> my_username;

  std::mutex pending_messages_mutex;
  std::shared_ptr<std::unordered_map<std::vector<uint8_t>,
                                     std::vector<std::vector<uint8_t>>>>
      pending_messages;

  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

  ClientContext(const std::string &server_ip, uint16_t port)
      : client(ClientFabric::create_tcp_client(server_ip, port)),
        mq(std::make_shared<MessageQueue>(client)),
        encryption_service(std::make_shared<EncryptionService>()),
        serializer(std::make_shared<Serializer>()),
        pending_files(std::make_shared<std::unordered_map<
                          std::string, std::unique_ptr<std::ofstream>>>()),
        pending_messages(
            std::make_shared<std::unordered_map<
                std::vector<uint8_t>, std::vector<std::vector<uint8_t>>>>()) {

    encryption_service->set_identity_key();
  }
};

struct ServerContext {
  std::shared_ptr<IServer> transport_server;
  std::shared_ptr<UserService> user_service;
  std::shared_ptr<ChatService> chat_service;
  std::shared_ptr<SessionManager> session_manager;
  Serializer serializer;

  int fd;

  Parser parser;
  Stream<std::pair<int, Message>> command_stream;

  ServerContext()
      : transport_server(ServerFabric::create_tcp_server()),
        user_service(std::make_shared<UserService>()),
        chat_service(std::make_shared<ChatService>()),
        session_manager(std::make_shared<SessionManager>()) {}
};
