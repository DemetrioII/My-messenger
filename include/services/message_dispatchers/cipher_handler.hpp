#pragma once
#include "../command_interface.hpp"
#include "../encryption_service.hpp"

class CipherMessageHandler : public IMessageHandler {
  std::shared_ptr<EncryptionService> encryption_service;
  void process_encrypted_message(const std::vector<uint8_t> &sender,
                                 const std::vector<uint8_t> &recipient,
                                 const std::vector<uint8_t> &pubkey_bytes,
                                 const std::vector<uint8_t> &payload,
                                 std::shared_ptr<ClientContext> context) {
    try {
      encryption_service->cache_public_key(sender, pubkey_bytes);
      auto decrypted_msg =
          encryption_service->decrypt_for(sender, recipient, payload);
      std::string plaintext(decrypted_msg.begin(), decrypted_msg.end());

      std::cout << "\n[Личное от " << std::string(sender.begin(), sender.end())
                << "]: " << plaintext << std::endl;
      context->ui_callback("[Личное от " +
                           std::string(sender.begin(), sender.end()) +
                           "]: " + plaintext);
    } catch (const std::exception &e) {
      std::cerr << "[Crypto] Decryption failed: " << e.what() << std::endl;
    }
  }

public:
  ~CipherMessageHandler() override;

  void handleMessageOnClient(const Message &msg,
                             std::shared_ptr<ClientContext> context) override;

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
