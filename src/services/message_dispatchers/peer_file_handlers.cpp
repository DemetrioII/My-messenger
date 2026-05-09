#include "../../../include/services/message_dispatchers/peer_file_handlers.hpp"

void PeerFileStartHandler::handleSending(const Message &msg,
                                         std::shared_ptr<PeerContext> context) {}

void PeerFileStartHandler::handleReceiving(const Message &msg,
                                           std::shared_ptr<PeerContext> context) {}

MessageType PeerFileStartHandler::getMessageType() const {
  return MessageType::FileStart;
}

void PeerFileChunkHandler::handleSending(const Message &msg,
                                         std::shared_ptr<PeerContext> context) {}

void PeerFileChunkHandler::handleReceiving(const Message &msg,
                                           std::shared_ptr<PeerContext> context) {}

MessageType PeerFileChunkHandler::getMessageType() const {
  return MessageType::FileChunk;
}

void PeerFileEndHandler::handleSending(const Message &msg,
                                       std::shared_ptr<PeerContext> context) {}

void PeerFileEndHandler::handleReceiving(const Message &msg,
                                         std::shared_ptr<PeerContext> context) {}

MessageType PeerFileEndHandler::getMessageType() const {
  return MessageType::FileEnd;
}
