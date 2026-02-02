#include "../../../include/services/commands_impl/exit.hpp"

CommandType ExitCommand::getType() const { return CommandType::EXIT; }

void ExitCommand::fromParsedCommand(const ParsedCommand &pc) {}

Message ExitCommand::toMessage() const {
  return Message(
      {}, 1, {std::vector<uint8_t>{static_cast<uint8_t>(CommandType::EXIT)}},
      MessageType::Command);
}

void ExitCommand::fromMessage(const Message &msg) {}

void ExitCommand::execeuteOnServer(std::shared_ptr<ServerContext> context) {
  auto user_service = context->user_service;
  auto session_manager = context->session_manager;
  auto chat_service = context->chat_service;
  auto username = session_manager->get_username(context->fd);
  user_service->remove_user(*username);
  chat_service->remove_user_from_all_chats(*username);
  session_manager->unbind(context->fd);
}

void ExitCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  auto client = context->client;
  client->send_to_server(context->serializer->serialize(toMessage()));
  client->disconnect();
}

ExitCommand::~ExitCommand() {}
