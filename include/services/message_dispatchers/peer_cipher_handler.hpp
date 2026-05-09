#pragma once

#include "../message_handler_interfaces.hpp"

class PeerCipherHandler : public IPeerMessageHandler {
public:
  void handleSending(const Message &msg,
                     std::shared_ptr<PeerContext> context) override;

  void handleReceiving(const Message &msg,
                       std::shared_ptr<PeerContext> context) override;

  MessageType getMessageType() const override;
};
