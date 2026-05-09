#pragma once

#include "include/application/dispatching/message_handler_interfaces.hpp"

class PeerResponseHandler : public IPeerMessageHandler {
  void handle_connect_response(const Message &msg,
                               const std::shared_ptr<PeerContext> context);

public:
  void handleSending(const Message &msg,
                     std::shared_ptr<PeerContext> context) override;

  void handleReceiving(const Message &msg,
                       std::shared_ptr<PeerContext> context) override;

  MessageType getMessageType() const override;
};
