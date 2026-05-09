#include "../../../include/services/message_dispatchers/peer_cipher_handler.hpp"

void PeerCipherHandler::handleSending(const Message &msg,
                                      std::shared_ptr<PeerContext> context) {
  std::cout << std::string_view(
                   reinterpret_cast<const char *>(context->my_username.data()),
                   context->my_username.size())
            << " sent message" << std::endl;
}

void PeerCipherHandler::handleReceiving(const Message &msg,
                                        std::shared_ptr<PeerContext> context) {
  auto sender = msg.get_meta(0);
  std::cout << std::string(context->my_username.begin(),
                           context->my_username.end())
            << " got message: [Private from "
            << std::string_view(reinterpret_cast<const char *>(sender.data()),
                                sender.size())
            << "]: ";
  auto encrypted_message = context->encryption_service->decrypt_for(
      sender, context->my_username, msg.get_payload(), msg.get_meta(1),
      context->messages_counter[sender]);
  std::cout << std::string_view(
                   reinterpret_cast<const char *>(encrypted_message.data()),
                   encrypted_message.size())
            << std::endl;
}

MessageType PeerCipherHandler::getMessageType() const {
  return MessageType::CipherMessage;
}
