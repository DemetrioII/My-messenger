#pragma once

#include "../message_handler_interfaces.hpp"

class ServerCipherHandler : public IServerMessageHandler {
public:
  void handleMessage(const Message &msg,
                     std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
