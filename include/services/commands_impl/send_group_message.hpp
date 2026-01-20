
#pragma once
#include "../command_interface.hpp"
#include "../encryption_service.hpp"

class SendGroupMessageCommand : public ICommandHandler {
  std::vector<uint8_t> sender;
  std::vector<uint8_t> recipient;
  std::vector<uint8_t> pubkey_bytes;
  std::vector<uint8_t> payload;

  void process_encrypted_message(EncryptionService *encryption_service) {
    try {
      encryption_service->cache_public_key(recipient, pubkey_bytes);
      auto decrypted_msg = encryption_service->decrypt_for(recipient, payload);
      std::string plaintext(decrypted_msg.begin(), decrypted_msg.end());

      std::cout << "\n[Личное]: " << plaintext << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "[Crypto] Decryption failed: " << e.what() << std::endl;
    }
  }

public:
  CommandType getType() const override { return CommandType::PRIVATE_MESSAGE; }

  void fromParsedCommand(const ParsedCommand &pc) override {}

  Message toMessage() const override {}

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override {
    // ДОПИСАТЬ
  }

  void executeOnClient(std::shared_ptr<ClientContext> context) override {
    auto encryption_service = context->encryption_service;
    // process_encrypted_message(encryption_service);
  }

  void fromMessage(const Message &msg) override {
    recipient = msg.get_meta(0);
    sender = msg.get_meta(1);
    payload = msg.get_payload();
    pubkey_bytes = msg.get_meta(2);
  }

  ~SendGroupMessageCommand() override {}
};
