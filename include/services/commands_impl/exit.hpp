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

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override {}

  void executeOnClient(std::shared_ptr<ClientContext> context) override {
    try {
      auto client = context->client;
      client->disconnect();
    } catch (const std::bad_any_cast &) {
      std::cerr << "Wrong argument type for ExitCommand" << std::endl;
    }
  }

  ~ExitCommand() override {}
};
