#pragma once

#include "include/application/dispatching/message_handler_interfaces.hpp"

class PeerFileStartHandler : public IPeerMessageHandler {
public:
  void handleSending(const Message &msg,
                     std::shared_ptr<PeerContext> context) override;

  void handleReceiving(const Message &msg,
                       std::shared_ptr<PeerContext> context) override;

  MessageType getMessageType() const override;
};

class PeerFileChunkHandler : public IPeerMessageHandler {
public:
  void handleSending(const Message &msg,
                     std::shared_ptr<PeerContext> context) override;

  void handleReceiving(const Message &msg,
                       std::shared_ptr<PeerContext> context) override;

  MessageType getMessageType() const override;
};

class PeerFileEndHandler : public IPeerMessageHandler {
public:
  void handleSending(const Message &msg,
                     std::shared_ptr<PeerContext> context) override;

  void handleReceiving(const Message &msg,
                       std::shared_ptr<PeerContext> context) override;

  MessageType getMessageType() const override;
};
