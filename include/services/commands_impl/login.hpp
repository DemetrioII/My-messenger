#pragma once
#include "../command_interface.hpp"
#include "../encryption_service.hpp"

class LoginCommand : public ICommandHandler {
  std::vector<uint8_t> username;
  std::vector<uint8_t> pubkey_bytes;

public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &parsed) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  ~LoginCommand() override;
};
