#pragma once

#include "../message_handler_interfaces.hpp"

class ServerFileStartHandler : public IServerMessageHandler {
public:
  void handleMessage(const Message &msg,
                     std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};

class ServerFileChunkHandler : public IServerMessageHandler {
public:
  void handleMessage(const Message &msg,
                     std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};

class ServerFileEndHandler : public IServerMessageHandler {
public:
  void handleMessage(const Message &msg,
                     std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
