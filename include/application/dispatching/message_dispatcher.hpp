#pragma once
#include "include/application/dispatching/message_handler_interfaces.hpp"
#include "include/application/session/client_context.hpp"
#include "include/models/message.hpp"
#include <iostream>
#include <unordered_map>

class ClientMessageDispatcher {
  std::unordered_map<MessageType, std::unique_ptr<IClientMessageHandler>>
      handlers;
  std::shared_ptr<ClientContext> context_;
  Serializer serializer;

public:
  void registerHandler(MessageType type,
                       std::unique_ptr<IClientMessageHandler> handler);

  void setContext(const std::shared_ptr<ClientContext> &context);

  bool dispatch(const Message &msg);

  bool dispatchFromRawBytes(const std::vector<uint8_t> &raw_data);

  bool HasHandlerFor(MessageType type) const;

  void clear();
};

class ServerMessageDispatcher {
  std::unordered_map<MessageType, std::unique_ptr<IServerMessageHandler>>
      handlers;
  std::shared_ptr<ServerContext> context_;
  Serializer serializer;

public:
  void registerHandler(MessageType type,
                       std::unique_ptr<IServerMessageHandler> handler);

  void setContext(const std::shared_ptr<ServerContext> &context);

  bool dispatch(const Message &msg);

  bool dispatchFromRawBytes(const std::vector<uint8_t> &raw_data);

  bool HasHandlerFor(MessageType type) const;

  void clear();
};

class PeerMessageDispatcher {
  std::unordered_map<MessageType, std::unique_ptr<IPeerMessageHandler>>
      handlers;
  std::shared_ptr<PeerContext> context_;
  Serializer serializer;

public:
  void registerHandler(MessageType type,
                       std::unique_ptr<IPeerMessageHandler> handler);

  void setContext(const std::shared_ptr<PeerContext> &context);

  bool dispatchSending(const Message &msg);

  bool dispatchReceiving(const Message &msg);

  bool dispatchFromRawBytes(const std::vector<uint8_t> &raw_data);

  bool hasHandleFor(MessageType type) const;

  void clear();
};
