#include "../../../include/services/message_dispatchers/cipher_handler.hpp"

CipherMessageHandler::~CipherMessageHandler() {}

void CipherMessageHandler::handleMessageOnClient(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  encryption_service = context->encryption_service;
  auto payload = msg.get_payload();
  auto sender_id = msg.get_meta(1);
  auto recipient_id = msg.get_meta(0);
  auto pubkey_opt = msg.get_meta(2);

  process_encrypted_message(sender_id, recipient_id, pubkey_opt, payload,
                            context);
}

void CipherMessageHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {
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
      {std::move(msg.get_payload()),
       3,
       {msg.get_meta(0), msg.get_meta(1), sender.get_public_key()},
       MessageType::CipherMessage});
  std::cout << "Message was sent to " << recipient_id << std::endl;
  context->transport_server->send(recipient_fd,
                                  context->serializer.serialize(msg_to_send));
}

MessageType CipherMessageHandler::getMessageType() const {
  return MessageType::CipherMessage;
}
