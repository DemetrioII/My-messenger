#include "include/application/dispatching/message_dispatchers/server_cipher_handler.hpp"

void ServerCipherHandler::handleMessage(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  context->app_service->deliver_cipher_message(context->fd, msg);
}

MessageType ServerCipherHandler::getMessageType() const {
  return MessageType::CipherMessage;
}
