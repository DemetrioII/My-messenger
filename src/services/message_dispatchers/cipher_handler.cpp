#include "../../../include/services/message_dispatchers/cipher_handler.hpp"

CipherMessageHandler::~CipherMessageHandler() {}

void CipherMessageHandler::process_encrypted_message(
    const std::vector<uint8_t> &sender, const std::vector<uint8_t> &recipient,
    const std::vector<uint8_t> &pubkey_bytes,
    const std::vector<uint8_t> &payload,
    const std::vector<uint8_t> &identity_key,
    const std::vector<uint8_t> &signature,
    std::shared_ptr<ClientContext> context) {
  try {
    auto encryption_service = context->encryption_service;
    encryption_service->cache_public_key(sender, pubkey_bytes, identity_key);
    auto counter_it = context->messages_counter.find(sender);
    if (counter_it == context->messages_counter.end()) {
      context->messages_counter[sender] = 0;
    }
    auto decrypted_msg =
        encryption_service->decrypt_for(sender, recipient, payload, signature,
                                        context->messages_counter[sender]);
    std::string plaintext(decrypted_msg.begin(), decrypted_msg.end());

    ++context->messages_counter[sender];

    std::cout << "\n[Личное от " << std::string(sender.begin(), sender.end())
              << "]: " << plaintext << std::endl;
    context->ui_callback("[Личное от " +
                         std::string(sender.begin(), sender.end()) +
                         "]: " + plaintext);
  } catch (const std::exception &e) {
    std::cerr << "[Crypto] Decryption failed: " << e.what() << std::endl;
  }
}

void CipherMessageHandler::handleIncoming(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  auto payload = msg.get_payload();
  auto sender_id = msg.get_meta(1);
  auto recipient_id = msg.get_meta(0);
  auto dh_pubkey_opt = msg.get_meta(2);
  auto identity_key = msg.get_meta(3);
  auto signauture = msg.get_meta(4);

  process_encrypted_message(sender_id, recipient_id, dh_pubkey_opt, payload,
                            identity_key, signauture, context);
}

void CipherMessageHandler::handleOutgoing(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  handleIncoming(msg, context);
}

MessageType CipherMessageHandler::getMessageType() const {
  return MessageType::CipherMessage;
}
