#include "include/application/dispatching/message_dispatchers/text_dispatcher.hpp"

void ClientTextHandler::handleIncoming(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  auto payload = msg.get_payload();
  std::string message(payload.begin(), payload.end());
  context->ui_callback(message);
  std::cout << "[Server]: " << message << std::endl;
}

void ClientTextHandler::handleOutgoing(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  auto payload = msg.get_payload();
  std::string message(payload.begin(), payload.end());
  context->ui_callback(message);
  std::cout << "[Client]: " << message << std::endl;
}

MessageType ClientTextHandler::getMessageType() const {
  return MessageType::Text;
}
