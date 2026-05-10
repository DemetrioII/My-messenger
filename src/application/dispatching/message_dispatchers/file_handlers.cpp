#include "include/application/dispatching/message_dispatchers/file_handlers.hpp"

MessageType ClientFileStartHandler::getMessageType() const {
  return MessageType::FileStart;
}

void ClientFileStartHandler::handleIncoming(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  auto pending_files = context->pending_files;
  uint64_t file_size = from_bytes(msg.get_meta(2));

  auto name = std::string(msg.get_meta(1).begin(), msg.get_meta(1).end());
  std::cout << "[File] Receiving " << name << " (" << file_size << " bytes)"
            << std::endl;

  std::string recipient_username_str =
      std::string(msg.get_meta(0).begin(), msg.get_meta(0).end());

  auto file_ptr = std::make_unique<std::ofstream>(
      recipient_username_str + " " + name, std::ios::binary);

  if (!file_ptr->is_open()) {
    std::cerr << "❌ Couldn't open the file!" << std::endl;
  } else {
    (*pending_files)[recipient_username_str + " " + name] = std::move(file_ptr);
  }
}

void ClientFileStartHandler::handleOutgoing(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  handleIncoming(msg, context);
}

MessageType ClientFileChunkHandler::getMessageType() const {
  return MessageType::FileChunk;
}

void ClientFileChunkHandler::handleIncoming(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  auto pending_files = context->pending_files;

  auto name = std::string(msg.get_meta(1).begin(), msg.get_meta(1).end());

  std::string recipient_username_str(msg.get_meta(0).begin(),
                                     msg.get_meta(0).end());

  if (pending_files->find(recipient_username_str + " " + name) !=
      pending_files->end()) {

    if (context->messages_counter.find(msg.get_meta(0)) ==
        context->messages_counter.end())
      context->messages_counter[msg.get_meta(0)] = 0;
    context->encryption_service->cache_public_key(
        msg.get_meta(2), msg.get_meta(3), msg.get_meta(4));

    std::vector<uint8_t> payload = context->encryption_service->decrypt_for(
        msg.get_meta(2), msg.get_meta(0), msg.get_payload(), msg.get_meta(5),
        context->messages_counter[msg.get_meta(0)]);
    (*pending_files)[recipient_username_str + " " + name]->write(
        reinterpret_cast<const char *>(payload.data()), payload.size());
  }

  std::cout << "[File] File chunk has been received " << name << std::endl;
}

void ClientFileChunkHandler::handleOutgoing(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  handleIncoming(msg, context);
}

MessageType ClientFileEndHandler::getMessageType() const {
  return MessageType::FileEnd;
}

void ClientFileEndHandler::handleIncoming(
    const Message &msg, const std::shared_ptr<ClientContext> context) {

  auto pending_files = context->pending_files;
  // auto fname = msg.get_meta(1);
  // auto recipient = msg.get_meta(0);
  std::string name =
      std::string(msg.get_meta(1).begin(), msg.get_meta(1).end());

  std::string recipient_username_str(msg.get_meta(0).begin(),
                                     msg.get_meta(0).end());

  auto key = recipient_username_str + " " + name;
  auto it = pending_files->find(key);
  if (it != pending_files->end()) {
    if (it->second) {
      it->second->close();
    }

    pending_files->erase(it);
    std::cout << "✅ [File] Completed and closed: " << name << std::endl;
  } else {
    std::cerr << "⚠️ [Warning] Attempted to close non-existent file: " << key
              << std::endl;
  }
}

void ClientFileEndHandler::handleOutgoing(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  handleIncoming(msg, context);
}
