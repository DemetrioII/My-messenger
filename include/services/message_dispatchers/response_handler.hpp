#pragma once
#include "../client_context.hpp"
#include "../encryption_service.hpp"
#include "../message_queue.hpp"

class ResponseMessageHandler : public IMessageHandler {
public:
  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override;
  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
