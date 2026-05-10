#pragma once

#include "include/application/session/client_context.hpp"
#include "include/models/message.hpp"

class IClientMessageHandler {
public:
  virtual ~IClientMessageHandler() = default;

  virtual void handleIncoming(const Message &msg,
                              const std::shared_ptr<ClientContext> context) = 0;
  virtual void handleOutgoing(const Message &msg,
                              const std::shared_ptr<ClientContext> context) = 0;
  virtual MessageType getMessageType() const = 0;
};

class IServerMessageHandler {
public:
  virtual ~IServerMessageHandler() = default;

  virtual void handleMessage(const Message &msg,
                             std::shared_ptr<ServerContext> context) = 0;
  virtual MessageType getMessageType() const = 0;
};

class IPeerMessageHandler {
public:
  virtual ~IPeerMessageHandler() = default;

  virtual void handleSending(const Message &msg,
                             std::shared_ptr<PeerContext> context) = 0;
  virtual void handleReceiving(const Message &msg,
                               std::shared_ptr<PeerContext> context) = 0;
  virtual MessageType getMessageType() const = 0;
};
