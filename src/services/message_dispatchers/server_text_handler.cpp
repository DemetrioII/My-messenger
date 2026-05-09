#include "../../../include/services/message_dispatchers/server_text_handler.hpp"

void ServerTextHandler::handleMessage(const Message &msg,
                                      std::shared_ptr<ServerContext> context) {
  auto bytes = msg.get_payload();
  auto payload = std::string(bytes.begin(), bytes.end());
  std::cout << "Client " << context->fd << " sent text: " << payload
            << std::endl;
}

MessageType ServerTextHandler::getMessageType() const {
  return MessageType::Text;
}
