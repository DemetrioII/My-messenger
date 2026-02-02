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
  auto user_service = context->user_service;
  auto chat_service = context->chat_service;
  auto session_manager = context->session_manager;

  auto username_res = session_manager->get_username(context->fd);
  if (!username_res.has_value()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }

  auto chat_name_str = std::string(chat_name.begin(), chat_name.end());
  if (chat_name_str.empty()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::CHAT_NOT_FOUND);
    return;
  }

  auto send_res =
      chat_service->post_message(chat_name_str, *username_res, toMessage());
  if (!send_res.has_value()) {
    if (send_res.error() == ServiceError::AccessDenied) {
      context->transport_server->send(
          context->fd, StaticResponses::CHAT_NOT_FOUND); // добавить обработчик
                                                         // ошибки доступа
      return;
    }
  }
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
