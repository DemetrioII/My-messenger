#pragma once
#include "../command_interface.hpp"
#include "../encryption_service.hpp"

class CipherMessageHandler : public IMessageHandler {
  std::shared_ptr<EncryptionService> encryption_service;
  void process_encrypted_message(const std::vector<uint8_t> &sender,
                                 const std::vector<uint8_t> &recipient,
                                 const std::vector<uint8_t> &pubkey_bytes,
                                 const std::vector<uint8_t> &payload,
                                 std::shared_ptr<ClientContext> context);

public:
  ~CipherMessageHandler() override;

  void handleMessageOnClient(const Message &msg,
                             std::shared_ptr<ClientContext> context) override;

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
