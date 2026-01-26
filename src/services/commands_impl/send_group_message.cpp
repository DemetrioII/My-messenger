#include "../../../include/services/commands_impl/send_group_message.hpp"

CommandType SendGroupMessageCommand::getType() const {
  return CommandType::SEND;
}

void SendGroupMessageCommand::fromParsedCommand(const ParsedCommand &pc) {}

Message SendGroupMessageCommand::toMessage() const {
  return Message{payload, 0, {}, MessageType::Text};
}

void SendGroupMessageCommand::execeuteOnServer(
    std::shared_ptr<ServerContext> context) {
  auto service = context->messaging_service;
  auto sender_id = service->get_user_id_by_fd(context->fd);
  if (sender_id.empty()) {
    std::string error_msg = "[Error]: You must logged in first";
    context->transport_server->send(
        context->fd,
        context->serializer.serialize(
            Message(std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0,
                    {}, MessageType::Text)));
    return;
  }

  auto chat_name_str = std::string(chat_name.begin(), chat_name.end());
  auto chat_id = service->get_chat_id_by_name(chat_name_str);
  if (chat_id.empty()) {
    std::string error_msg = "[Error]: Chat not found";
    context->transport_server->send(
        context->fd,
        context->serializer.serialize(
            Message(std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0,
                    {}, MessageType::Text)));
    return;
  }

  if (!service->is_member_of_chat(chat_name_str, sender_id)) {
    std::string error_msg = "[Error]: You must be a member of that chat!";
    context->transport_server->send(
        context->fd,
        context->serializer.serialize(
            Message(std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0,
                    {}, MessageType::Text)));
    return;
  }

  service->send_message(sender_id, chat_id, toMessage());
}

void SendGroupMessageCommand::executeOnClient(
    std::shared_ptr<ClientContext> context) {
  auto encryption_service = context->encryption_service;
  auto client = context->client;
  client->send_to_server(context->serializer->serialize(Message(
      payload, 2, {{static_cast<uint8_t>(CommandType::SEND)}, chat_name},
      MessageType::Command)));
}

void SendGroupMessageCommand::fromMessage(const Message &msg) {
  chat_name = msg.get_meta(1);
  payload = msg.get_payload();
}

SendGroupMessageCommand::~SendGroupMessageCommand() {}
