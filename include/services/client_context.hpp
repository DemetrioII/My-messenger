#pragma once
#include "../network/protocol/parser.hpp"
#include "../network/transport/interface.hpp"
#include "../network/transport/server.hpp"
#include "encryption_service.hpp"
#include "message_queue.hpp"
#include "messageing_service.hpp"

struct ClientContext {

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

  ClientContext()
      : client(Client::create()), mq(std::make_shared<MessageQueue>(client)),
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
  std::shared_ptr<Server> transport_server;
  std::shared_ptr<MessagingService> messaging_service;
  Serializer serializer;

  int fd;

  Parser parser;
  Stream<std::pair<int, Message>> command_stream;

  ServerContext()
      : transport_server(Server::create()),
        messaging_service(std::make_shared<MessagingService>()) {}
};
