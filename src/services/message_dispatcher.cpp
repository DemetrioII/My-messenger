#include "../../../include/services/message_dispatcher.hpp"

void ClientMessageDispatcher::registerHandler(
    MessageType type, std::unique_ptr<IMessageHandler> handler) {
  handlers[type] = std::move(handler);
}

void ClientMessageDispatcher::setContext(
    const std::shared_ptr<ClientContext> &context) {
  context_ = context;
}
bool ClientMessageDispatcher::dispatch(const Message &msg) {
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

bool ClientMessageDispatcher::dispatchFromRawBytes(
    const std::vector<uint8_t> &raw_data) {
  try {
    Message msg = serializer.deserialize(raw_data);
    return dispatch(msg);
  } catch (const std::exception &e) {
    std::cerr << "[Dispatcher] Failed to deserialize message: " << e.what()
              << std::endl;
    return false;
  }
}

bool ClientMessageDispatcher::HasHandlerFor(MessageType type) const {
  auto it = handlers.find(type);
  return it != handlers.end();
}

void ClientMessageDispatcher::clear() { handlers.clear(); }

void ServerMessageDispatcher::registerHandler(
    MessageType type, std::unique_ptr<IMessageHandler> handler) {
  handlers[type] = std::move(handler);
}

void ServerMessageDispatcher::setContext(
    const std::shared_ptr<ServerContext> &context) {
  context_ = context;
}
bool ServerMessageDispatcher::dispatch(const Message &msg) {
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

bool ServerMessageDispatcher::dispatchFromRawBytes(
    const std::vector<uint8_t> &raw_data) {
  try {
    Message msg = serializer.deserialize(raw_data);
    return dispatch(msg);
  } catch (const std::exception &e) {
    std::cerr << "[Dispatcher] Failed to deserialize message: " << e.what()
              << std::endl;
    return false;
  }
}

bool ServerMessageDispatcher::HasHandlerFor(MessageType type) const {
  auto it = handlers.find(type);
  return it != handlers.end();
}

void ServerMessageDispatcher::clear() { handlers.clear(); }
