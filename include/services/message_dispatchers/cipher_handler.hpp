#pragma once
#include "../command_interface.hpp"
#include "../encryption_service.hpp"
#include "../message_handler_interfaces.hpp"

class CipherMessageHandler : public IClientMessageHandler {
  void process_encrypted_message(const std::vector<uint8_t> &sender,
                                 const std::vector<uint8_t> &recipient,
                                 const std::vector<uint8_t> &pubkey_bytes,
                                 const std::vector<uint8_t> &payload,
                                 const std::vector<uint8_t> &identity_key,
                                 const std::vector<uint8_t> &signauture,
                                 std::shared_ptr<ClientContext> context);

public:
  ~CipherMessageHandler() override;

  void handleIncoming(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  void handleOutgoing(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  MessageType getMessageType() const override;
};
