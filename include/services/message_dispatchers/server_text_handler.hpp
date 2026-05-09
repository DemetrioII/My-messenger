#pragma once

#include "../message_handler_interfaces.hpp"

class ServerTextHandler : public IServerMessageHandler {
public:
  void handleMessage(const Message &msg,
                     std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
