#pragma once
#include "../message_dispatcher.hpp"

class TextMessageHandler : public IMessageHandler {
public:
  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override {
    auto payload = msg.get_payload();
    std::string message(payload.begin(), payload.end());
    context->ui_callback(message);
    std::cout << "[Server]: " << message << std::endl;
  }

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override {
    auto bytes = msg.get_payload();
    auto payload = std::string(bytes.begin(), bytes.end());
    std::cout << "Client " << context->fd << " sent message: " << payload
              << std::endl;
  }

  MessageType getMessageType() const override { return MessageType::Text; }
};
