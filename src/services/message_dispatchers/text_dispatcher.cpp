#include "../../../include/services/message_dispatchers/text_dispatcher.hpp"

void TextMessageHandler::handleMessageOnClient(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  auto payload = msg.get_payload();
  std::string message(payload.begin(), payload.end());
  context->ui_callback(message);
  std::cout << "[Server]: " << message << std::endl;
}

void TextMessageHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  auto bytes = msg.get_payload();
  auto payload = std::string(bytes.begin(), bytes.end());
  std::cout << "Client " << context->fd << " sent message: " << payload
            << std::endl;
}

MessageType TextMessageHandler::getMessageType() const {
  return MessageType::Text;
}
