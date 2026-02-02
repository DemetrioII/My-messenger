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
  auto username = context->session_manager->get_username(context->fd);
  if (!username.has_value()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }
  auto recipient_username =
      std::string(msg.get_meta(0).begin(), msg.get_meta(0).end());
  auto recipient_fd = context->session_manager->get_fd(recipient_username);
  if (!recipient_fd.has_value()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::USER_NOT_FOUND);
    return;
  }
  auto sender = context->user_service->find_user(*username);
  if (!sender.has_value()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }
  Message msg_to_send(
      {std::move(msg.get_payload()),
       3,
       {msg.get_meta(0), msg.get_meta(1), (*sender)->get_public_key()},
       MessageType::CipherMessage});
  std::cout << "Message was sent to " << recipient_username << std::endl;
  context->transport_server->send(*recipient_fd,
                                  context->serializer.serialize(msg_to_send));
}

MessageType CipherMessageHandler::getMessageType() const {
  return MessageType::CipherMessage;
}
