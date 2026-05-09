#pragma once
#include "../client_context.hpp"
#include "../encryption_service.hpp"
#include "../message_handler_interfaces.hpp"
#include "../message_queue.hpp"

class ResponseMessageHandler : public IClientMessageHandler {
private:
  void handle_get_pubkey_response(const Message &msg,
                                  const std::shared_ptr<ClientContext> context);
  void handle_connect_response(const Message &msg,
                               const std::shared_ptr<PeerContext> context);

public:
  void handleIncoming(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;
  void handleOutgoing(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  MessageType getMessageType() const override;
};
