#include "include/application/commands/impl/send_group_message.hpp"

CommandType SendGroupMessageCommand::getType() const {
  return CommandType::SEND;
}

void SendGroupMessageCommand::fromParsedCommand(const ParsedCommand &pc) {}

Message SendGroupMessageCommand::toMessage() const {
  return Message{payload, 0, {}, MessageType::Text};
}

void SendGroupMessageCommand::execeuteOnServer(
    std::shared_ptr<ServerContext> context) {
  auto chat_name_str = std::string(chat_name.begin(), chat_name.end());
  context->app_service->send_group_message(chat_name_str, toMessage());
}

void SendGroupMessageCommand::executeOnClient(
    std::shared_ptr<ClientContext> context) {
  auto encryption_service = context->encryption_service;
  auto client = context->client;
  client->send_to_server(context->serializer->serialize(Message(
      payload, 2, {{static_cast<uint8_t>(CommandType::SEND)}, chat_name},
      MessageType::Command)));
}

void SendGroupMessageCommand::send_from_peer(
    std::shared_ptr<PeerContext> context) {}

void SendGroupMessageCommand::recv_on_peer(
    int fd, std::shared_ptr<PeerContext> context) {}

void SendGroupMessageCommand::fromMessage(const Message &msg) {
  chat_name = msg.get_meta(1);
  payload = msg.get_payload();
}

SendGroupMessageCommand::~SendGroupMessageCommand() {}
