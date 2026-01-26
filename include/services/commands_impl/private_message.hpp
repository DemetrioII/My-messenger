#pragma once
#include "../command_interface.hpp"
#include "../encryption_service.hpp"
#include "../message_queue.hpp"
#include "get_pubkey.hpp"

class PrivateMessageCommand : public ICommandHandler {
  std::vector<uint8_t> sender;
  std::vector<uint8_t> recipient;
  std::vector<uint8_t> pubkey_bytes;
  std::vector<uint8_t> payload;

  void process_encrypted_message(
      std::shared_ptr<EncryptionService> encryption_service);

public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &parsed) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  ~PrivateMessageCommand() override;
};
