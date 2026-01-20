#pragma once
#include "../encryption/AESGCMEncryption.hpp"
#include "../encryption/identity_key.hpp"
#include "../models/chat.hpp"
#include "../models/user.hpp"
#include "../network/protocol/parser.hpp"
#include "../network/transport/client.hpp"
#include "../network/transport/server.hpp"
#include "command_interface.hpp"
#include "commands_impl/get_pubkey.hpp"
#include "commands_impl/join.hpp"
#include "commands_impl/login.hpp"
#include "commands_impl/make_room.hpp"
#include "commands_impl/sendfile.hpp"
#include "message_dispatcher.hpp"
#include "message_dispatchers/text_dispatcher.hpp"

#include "../utils/base64.hpp"
#include "client_context.hpp"
#include "encryption_service.hpp"
#include "message_dispatchers/cipher_handler.hpp"
#include "message_dispatchers/command_handler.hpp"
#include "message_dispatchers/file_handlers.hpp"
#include "message_dispatchers/response_handler.hpp"

#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

class MessagingServer {
private:
  std::shared_ptr<ServerContext> server_context;
  ServerMessageDispatcher dispatcher;

  static uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessagingServer() : server_context(std::make_shared<ServerContext>()) {
    dispatcher.registerHandler(MessageType::Text,
                               std::make_unique<TextMessageHandler>());

    dispatcher.registerHandler(MessageType::Command,
                               std::make_unique<MessageCommandHandler>());

    dispatcher.registerHandler(MessageType::CipherMessage,
                               std::make_unique<CipherMessageHandler>());

    dispatcher.registerHandler(MessageType::FileStart,
                               std::make_unique<FileStartHandler>());

    dispatcher.registerHandler(MessageType::FileChunk,
                               std::make_unique<FileChunkHandler>());

    dispatcher.registerHandler(MessageType::FileEnd,
                               std::make_unique<FileEndHandler>());

    server_context->messaging_service->user_logged_in.subscribe(
        [](const User &) { return true; },
        [this](const User &user) {
          int fd = server_context->messaging_service->get_fd_by_user_id(
              user.get_id());
          Message welcome_msg =
              server_context->parser.parse("Hello, " + user.get_id() + "!");
          server_context->transport_server->send(
              fd, server_context->serializer.serialize(welcome_msg));
        });

    server_context->messaging_service->message_sent.subscribe(
        [](auto &) { return true; },
        [this](auto &tpl) {
          auto [from_user_id, chat_id, msg] = tpl;
          auto &chat =
              server_context->messaging_service->get_chat_by_id(chat_id);
          for (auto &member : chat.get_members()) {
            int fd =
                server_context->messaging_service->get_fd_by_user_id(member);
            Message m = server_context->parser.parse(
                "Message from " + from_user_id + " in chat " + chat_id + " ");
            server_context->transport_server->send(
                fd, server_context->serializer.serialize(m));
          }
        });
  }

  void start_server(int port) {
    server_context->transport_server->start(port);
    server_context->transport_server->set_data_callback(
        [this](int fd, auto data) { on_tcp_data_received(fd, data); });
  }

  void run() { server_context->transport_server->run_event_loop(); }

  void on_tcp_data_received(int fd, std::vector<uint8_t> &raw_data) {
    Message msg = server_context->serializer.deserialize(raw_data);

    server_context->fd = fd;
    dispatcher.setContext(server_context);
    dispatcher.dispatch(msg);
  }

  void send_error(int fd, const std::string &error_string) {
    server_context->transport_server->send(fd, {'e', 'r', 'r', 'o', 'r'});
  }
};
