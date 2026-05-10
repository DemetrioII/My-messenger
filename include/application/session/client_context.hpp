#pragma once
#include "include/app/config.hpp"
#include "include/application/messaging/message_queue.hpp"
#include "include/application/services/chat_service.hpp"
#include "include/application/services/encryption_service.hpp"
#include "include/application/services/server_application_service.hpp"
#include "include/application/services/session_manager.hpp"
#include "include/application/services/user_service.hpp"
#include "include/models/message.hpp"
#include "include/transport/peer.hpp"
#include <fstream>
#include <functional>

struct ClientContext {
  AppConfig config;
  std::function<void(const std::string &)> ui_callback = nullptr;
  std::shared_ptr<IClient> client;
  std::shared_ptr<MessageQueue> mq;

  std::shared_ptr<EncryptionService> encryption_service;
  Parser parser;
  std::shared_ptr<Serializer> serializer;
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

  std::mutex pending_messages_mutex;
  std::shared_ptr<std::unordered_map<std::vector<uint8_t>,
                                     std::vector<std::vector<uint8_t>>>>
      pending_messages;

  std::vector<uint8_t> my_username;
  std::unordered_map<std::vector<uint8_t>, uint64_t> messages_counter;

  ClientContext(const AppConfig &cfg)
      : config(cfg), client(ClientFactory::tcp_client(config.server_host,
                                                      config.server_port)),
        mq(std::make_shared<MessageQueue>(client)),
        encryption_service(std::make_shared<EncryptionService>()),
        serializer(std::make_shared<Serializer>()),
        pending_files(std::make_shared<std::unordered_map<
                          std::string, std::unique_ptr<std::ofstream>>>()),
        pending_messages(
            std::make_shared<std::unordered_map<
                std::vector<uint8_t>, std::vector<std::vector<uint8_t>>>>()) {

    encryption_service->set_key();
  }
};

struct PeerContext {
  std::shared_ptr<PeerNode> peer_node;
  std::shared_ptr<UserService> user_service;
  std::shared_ptr<SessionManager> session_manager;
  std::shared_ptr<std::unordered_map<std::vector<uint8_t>,
                                     std::vector<std::vector<uint8_t>>>>
      pending_messages;
  std::shared_ptr<Serializer> serializer;
  std::shared_ptr<EncryptionService> encryption_service;
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;
  std::shared_ptr<MessageQueue> message_queue;
  Parser parser;
  std::vector<uint8_t> my_username;
  std::unordered_map<std::vector<uint8_t>, uint64_t> messages_counter;
  std::mutex pending_messages_mutex;

  int fd;

  PeerContext()
      : peer_node(PeerNodeFactory::create_tcp_peer()),
        user_service(std::make_shared<UserService>()),
        session_manager(std::make_shared<SessionManager>()),
        pending_messages(
            std::make_shared<std::unordered_map<
                std::vector<uint8_t>, std::vector<std::vector<uint8_t>>>>()),
        serializer(std::make_shared<Serializer>()),
        encryption_service(std::make_shared<EncryptionService>()),
        pending_files(std::make_shared<std::unordered_map<
                          std::string, std::unique_ptr<std::ofstream>>>()) {
    encryption_service->set_key();
  }
};

struct ServerContext {
  AppConfig config;
  std::shared_ptr<IServer> transport_server;
  std::shared_ptr<UserService> user_service;
  std::shared_ptr<ChatService> chat_service;
  std::shared_ptr<SessionManager> session_manager;
  std::shared_ptr<ServerApplicationService> app_service;
  Serializer serializer;

  int fd;

  Parser parser;
  Stream<std::pair<int, Message>> command_stream;

  ServerContext(const AppConfig &cfg)
      : config(cfg), transport_server(ServerFactory::tcp_server(
                         config.server_host, config.server_port)),
        user_service(std::make_shared<UserService>()),
        chat_service(std::make_shared<ChatService>()),
        session_manager(std::make_shared<SessionManager>()) {
    app_service = std::make_shared<ServerApplicationService>(
        user_service, chat_service, session_manager, transport_server,
        &serializer, &fd);
  }
};
