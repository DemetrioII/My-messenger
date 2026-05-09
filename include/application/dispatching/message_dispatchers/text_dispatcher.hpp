#pragma once
#include "include/application/dispatching/message_handler_interfaces.hpp"

class ClientTextHandler : public IClientMessageHandler {
public:
  void handleIncoming(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;
  void handleOutgoing(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  MessageType getMessageType() const override;
};
