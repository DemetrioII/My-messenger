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
      std::shared_ptr<EncryptionService> encryption_service) {
    try {
      encryption_service->cache_public_key(recipient, pubkey_bytes);
      auto decrypted_msg =
          encryption_service->decrypt_for(sender, recipient, payload);
      std::string plaintext(decrypted_msg.begin(), decrypted_msg.end());

      std::string sender_username(sender.begin(), sender.end());
      std::cout << "\n[Личное]: (" << sender_username << "): " << plaintext
                << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "[Crypto] Decryption failed: " << e.what() << std::endl;
    }
  }

public:
  CommandType getType() const override { return CommandType::PRIVATE_MESSAGE; }

  void fromParsedCommand(const ParsedCommand &pc) override {
    if (pc.args.size() > 1) {
      recipient = pc.args[0];
      payload = pc.args[1];
    }
  }

  Message toMessage() const override {}

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override {}

  void executeOnClient(std::shared_ptr<ClientContext> context) override {
    auto encryption_service = context->encryption_service;
    auto mq = context->mq;
    if (recipient.empty() || payload.empty()) {
      std::cout << "Usage: /pmess <recipient> <payload>" << std::endl;
      return;
    }
    GetPubkeyCommand cmd;
    ParsedCommand pc{.name = "getpub", .args{recipient}};
    cmd.fromParsedCommand(pc);
    cmd.executeOnClient(context);
    mq->push(recipient, payload);
    recipient.clear();
    payload.clear();
    // process_encrypted_message(encryption_service);
  }

  void fromMessage(const Message &msg) override {
    recipient = msg.get_meta(1);
    // sender = msg.get_meta(1);
    payload = msg.get_payload();
    // pubkey_bytes = msg.get_meta(2);
  }

  ~PrivateMessageCommand() override {}
};
