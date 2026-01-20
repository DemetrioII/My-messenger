#pragma once
#include "../command_interface.hpp"

class ExitCommand : public ICommandHandler {
public:
  CommandType getType() const override { return CommandType::EXIT; }

  void fromParsedCommand(const ParsedCommand &pc) override {}

  Message toMessage() const override {
    return Message(
        {}, 1, {std::vector<uint8_t>{static_cast<uint8_t>(CommandType::EXIT)}},
        MessageType::Command);
  }

  void fromMessage(const Message &msg) override {}

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override {
    auto service = context->messaging_service;
    service->remove_user_by_fd(context->fd);
  }

  void executeOnClient(std::shared_ptr<ClientContext> context) override {
    auto client = context->client;
    client->send_to_server(context->serializer->serialize(toMessage()));
    client->disconnect();
  }

  ~ExitCommand() override {}
};
