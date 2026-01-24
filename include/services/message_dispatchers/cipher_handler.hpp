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
      encryption_service->cache_public_key(recipient, pubkey_bytes);
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
  ~CipherMessageHandler() override {}

  void handleMessageOnClient(const Message &msg,
                             std::shared_ptr<ClientContext> context) override {
    encryption_service = context->encryption_service;
    auto payload = msg.get_payload();
    auto sender_id = msg.get_meta(1);
    auto recipient_id = msg.get_meta(0);
    auto pubkey_opt = msg.get_meta(2);

    process_encrypted_message(sender_id, recipient_id, pubkey_opt, payload,
                              context);
  }

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override {
    std::string user_id =
        context->messaging_service->get_user_id_by_fd(context->fd);
    auto recipient_username = msg.get_meta(0);
    auto recipient_id =
        std::string(recipient_username.begin(), recipient_username.end());
    recipient_id = context->messaging_service->get_user_by_name(recipient_id);
    auto recipient_fd =
        context->messaging_service->get_fd_by_user_id(recipient_id);
    auto &sender = context->messaging_service->get_user_by_id(user_id);
    Message msg_to_send(
        {msg.get_payload(),
         3,
         {msg.get_meta(0), msg.get_meta(1), sender.get_public_key()},
         MessageType::CipherMessage});
    std::cout << "Message was sent to " << recipient_id << std::endl;
    context->transport_server->send(recipient_fd,
                                    context->serializer.serialize(msg_to_send));
  }

  MessageType getMessageType() const override {
    return MessageType::CipherMessage;
  }
};
