#pragma once
#include "../command_interface.hpp"

class ExitCommand : public ICommandHandler {
public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &pc) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  ~ExitCommand() override;
};
