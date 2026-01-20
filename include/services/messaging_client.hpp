#pragma once

#include "../network/protocol/parser.hpp"

#include "message_dispatcher.hpp"
#include "message_dispatchers/text_dispatcher.hpp"

#include "../utils/base64.hpp"
#include "client_context.hpp"
#include "encryption_service.hpp"
#include "message_dispatchers/cipher_handler.hpp"
#include "message_dispatchers/command_handler.hpp"
#include "message_dispatchers/file_handlers.hpp"
#include "message_dispatchers/response_handler.hpp"
#include "message_queue.hpp"
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

class MessagingClient {
  ClientMessageDispatcher dispatcher;
  std::shared_ptr<ClientContext> context;

private:
  static uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

  void handle_command(const std::string &msg) {
    ParsedCommand cmd_struct = context->parser.parse_line(msg);
  }

public:
  MessagingClient() : context(std::make_shared<ClientContext>()) {
    dispatcher.registerHandler(MessageType::Text,
                               std::make_unique<TextMessageHandler>());
    dispatcher.registerHandler(MessageType::Command,
                               std::make_unique<MessageCommandHandler>());
    dispatcher.registerHandler(MessageType::Response,
                               std::make_unique<ResponseMessageHandler>(
                                   context->my_username,
                                   context->encryption_service, context->mq,
                                   context->client, context->serializer));

    dispatcher.registerHandler(MessageType::CipherMessage,
                               std::make_unique<CipherMessageHandler>());
    dispatcher.registerHandler(MessageType::FileStart,
                               std::make_unique<FileStartHandler>());
    dispatcher.registerHandler(MessageType::FileChunk,
                               std::make_unique<FileChunkHandler>());
    dispatcher.registerHandler(MessageType::FileEnd,
                               std::make_unique<FileEndHandler>());
  }

  int init_client(const std::string &server_ip, int port) {
    if (!context->client->connect(server_ip, port))
      return 0;
    context->client->set_data_callback(
        [this](const std::vector<uint8_t> &data) {
          on_tcp_data_received(data);
        });
    return 1;
  }

  void on_tcp_data_received(const std::vector<uint8_t> &raw_data) {
    Message msg = context->serializer->deserialize(raw_data);
    dispatcher.setContext(context);
    dispatcher.dispatch(msg);
  }

  void get_data(const std::string &data) {
    Message msg = context->parser.parse(data);
    auto bytes = context->serializer->serialize(msg);
    if (msg.get_type() == MessageType::Command) {
      dispatcher.setContext(context);
      dispatcher.dispatch(msg);
    } else if (msg.get_type() == MessageType::Text) {
      context->client->send_to_server(bytes);
    }
  }

  void run() { context->client->run_event_loop(); }
};
