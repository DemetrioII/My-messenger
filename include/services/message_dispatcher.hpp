#pragma once
#include "../models/message.hpp"
#include "client_context.hpp"
#include <iostream>
#include <unordered_map>

class ClientMessageDispatcher {
  std::unordered_map<MessageType, std::unique_ptr<IMessageHandler>> handlers;
  std::shared_ptr<ClientContext> context_;
  Serializer serializer;

public:
  void registerHandler(MessageType type,
                       std::unique_ptr<IMessageHandler> handler);

  void setContext(const std::shared_ptr<ClientContext> &context);

  bool dispatch(const Message &msg);

  bool dispatchFromRawBytes(const std::vector<uint8_t> &raw_data);

  bool HasHandlerFor(MessageType type) const;

  void clear();
};

class ServerMessageDispatcher {
  std::unordered_map<MessageType, std::unique_ptr<IMessageHandler>> handlers;
  std::shared_ptr<ServerContext> context_;
  Serializer serializer;

public:
  void registerHandler(MessageType type,
                       std::unique_ptr<IMessageHandler> handler);

  void setContext(const std::shared_ptr<ServerContext> &context);

  bool dispatch(const Message &msg);

  bool dispatchFromRawBytes(const std::vector<uint8_t> &raw_data);

  bool HasHandlerFor(MessageType type) const;

  void clear();
};
