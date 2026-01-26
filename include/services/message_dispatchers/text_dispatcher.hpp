#pragma once
#include "../message_dispatcher.hpp"

class TextMessageHandler : public IMessageHandler {
public:
  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override;

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
