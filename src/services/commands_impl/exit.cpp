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
  auto service = context->messaging_service;
  service->remove_user_by_fd(context->fd);
}

void ExitCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  auto client = context->client;
  client->send_to_server(context->serializer->serialize(toMessage()));
  client->disconnect();
}

ExitCommand::~ExitCommand() {}
