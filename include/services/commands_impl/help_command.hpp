#pragma once
#include "../command_interface.hpp"

class HelpCommand : public ICommandHandler {
public:
  ~HelpCommand() override {}

  void fromParsedCommand(const ParsedCommand &parsed) override {}

  Message toMessage() const override {}

  void fromMessage(const Message &msg) override {}

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override {}

  void executeOnClient(std::shared_ptr<ClientContext> context) override {}

  void send_from_peer(int fd, std::shared_ptr<PeerContext> context) override {}

  void recv_on_peer(int fd, std::shared_ptr<PeerContext> context) override {}

  bool isClientOnly() const override { return true; }

  CommandType getType() const override { return CommandType::HELP; }
};
