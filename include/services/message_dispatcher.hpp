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
                       std::unique_ptr<IMessageHandler> handler) {
    handlers[type] = std::move(handler);
  }

  void setContext(const std::shared_ptr<ClientContext> &context) {
    context_ = context;
  }
  bool dispatch(const Message &msg) {
    auto it = handlers.find(msg.get_type());
    if (it == handlers.end()) {
      std::cerr << "[MessageDispatcher] No handler for message type: "
                << static_cast<int>(msg.get_type()) << std::endl;
      return false;
    }

    bool handled = false;
    try {
      it->second->handleMessageOnClient(msg, context_);
      handled = true;
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      handled = false;
    }

    return handled;
  }

  bool dispatchFromRawBytes(const std::vector<uint8_t> &raw_data) {
    try {
      Message msg = serializer.deserialize(raw_data);
      return dispatch(msg);
    } catch (const std::exception &e) {
      std::cerr << "[Dispatcher] Failed to deserialize message: " << e.what()
                << std::endl;
      return false;
    }
  }

  bool HasHandlerFor(MessageType type) const {
    auto it = handlers.find(type);
    return it != handlers.end();
  }

  void clear() { handlers.clear(); }
};

class ServerMessageDispatcher {
  std::unordered_map<MessageType, std::unique_ptr<IMessageHandler>> handlers;
  std::shared_ptr<ServerContext> context_;
  Serializer serializer;

public:
  void registerHandler(MessageType type,
                       std::unique_ptr<IMessageHandler> handler) {
    handlers[type] = std::move(handler);
  }

  void setContext(const std::shared_ptr<ServerContext> &context) {
    context_ = context;
  }
  bool dispatch(const Message &msg) {
    auto it = handlers.find(msg.get_type());
    if (it == handlers.end()) {
      std::cerr << "[MessageDispatcher] No handler for message type: "
                << static_cast<int>(msg.get_type()) << std::endl;
      return false;
    }

    bool handled = false;
    try {
      it->second->handleMessageOnServer(msg, context_);
      handled = true;
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      handled = false;
    }

    return handled;
  }

  bool dispatchFromRawBytes(const std::vector<uint8_t> &raw_data) {
    try {
      Message msg = serializer.deserialize(raw_data);
      return dispatch(msg);
    } catch (const std::exception &e) {
      std::cerr << "[Dispatcher] Failed to deserialize message: " << e.what()
                << std::endl;
      return false;
    }
  }

  bool HasHandlerFor(MessageType type) const {
    auto it = handlers.find(type);
    return it != handlers.end();
  }

  void clear() { handlers.clear(); }
};
