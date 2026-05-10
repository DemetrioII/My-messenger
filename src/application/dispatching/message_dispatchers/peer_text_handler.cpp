#include "include/application/dispatching/message_dispatchers/peer_text_handler.hpp"

void PeerTextHandler::handleSending(const Message &msg,
                                    std::shared_ptr<PeerContext> context) {
  broadcast(*context->peer_node, context->serializer->serialize(msg));
}

void PeerTextHandler::handleReceiving(const Message &msg,
                                      std::shared_ptr<PeerContext> /*context*/) {
  std::cout << "Peer got message "
            << std::string(msg.get_payload().begin(), msg.get_payload().end())
            << std::endl;
}

MessageType PeerTextHandler::getMessageType() const {
  return MessageType::Text;
}
