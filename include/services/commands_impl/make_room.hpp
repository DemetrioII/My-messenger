#pragma once
#include "../command_interface.hpp"

class MakeRoomCommand : public ICommandHandler {
  std::vector<uint8_t> chat_name;

public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &parsed) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  bool isClientOnly() const override;

  ~MakeRoomCommand() override;
};
