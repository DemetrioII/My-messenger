
#pragma once
#include "../command_interface.hpp"
#include "../encryption_service.hpp"

class SendGroupMessageCommand : public ICommandHandler {
  std::vector<uint8_t> sender;
  std::vector<uint8_t> chat_name;
  std::vector<uint8_t> pubkey_bytes;
  std::vector<uint8_t> payload;

public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &parsed) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  ~SendGroupMessageCommand() override;
};
