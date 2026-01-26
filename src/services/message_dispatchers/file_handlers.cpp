#include "../../../include/services/message_dispatchers/file_handlers.hpp"

MessageType FileStartHandler::getMessageType() const {
  return MessageType::FileStart;
}

void FileStartHandler::handleMessageOnClient(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  pending_files = context->pending_files;
  auto recipient = msg.get_meta(0);
  auto fname = msg.get_meta(1);
  auto size_bytes = msg.get_meta(2);
  uint64_t file_size = from_bytes(size_bytes);

  auto name = std::string(fname.begin(), fname.end());
  std::cout << "[File] Receiving " << name << " (" << file_size << " bytes)"
            << std::endl;

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  auto file_ptr = std::make_unique<std::ofstream>(
      recipient_username_str + " " + name, std::ios::binary);

  if (!file_ptr->is_open()) {
    std::cerr << "❌ Couldn't open the file!" << std::endl;
  } else {
    (*pending_files)[recipient_username_str + " " + name] = std::move(file_ptr);
  }
}

void FileStartHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  auto recipient = msg.get_meta(0);
  auto fname = msg.get_meta(1);
  auto size_bytes = msg.get_meta(2);
  uint64_t file_size = from_bytes(size_bytes);

  auto name = std::string(fname.begin(), fname.end());
  std::cout << "[File] Server Receiving " << name << " (" << file_size
            << " bytes)" << std::endl;

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  int recipient_fd = context->messaging_service->get_fd_by_user_id(
      context->messaging_service->get_user_by_name(recipient_username_str));
  context->transport_server->send(recipient_fd,
                                  context->serializer.serialize(msg));
}

MessageType FileChunkHandler::getMessageType() const {
  return MessageType::FileChunk;
}

void FileChunkHandler::handleMessageOnClient(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  pending_files = context->pending_files;
  auto recipient = msg.get_meta(0);
  auto fname = msg.get_meta(1);
  auto sender = msg.get_meta(2);
  auto name = std::string(fname.begin(), fname.end());

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  if (pending_files->find(recipient_username_str + " " + name) !=
      pending_files->end()) {

    auto payload = msg.get_payload();
    std::vector<uint8_t> cipher_bytes(payload.begin(), payload.end());
    payload = context->encryption_service->decrypt_for(sender, recipient,
                                                       cipher_bytes);
    (*pending_files)[recipient_username_str + " " + name]->write(
        reinterpret_cast<const char *>(payload.data()), payload.size());
  }

  std::cout << "[File] File chunk has been received " << name << std::endl;
}

void FileChunkHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  auto recipient = msg.get_meta(0);
  auto fname = msg.get_meta(1);
  auto name = std::string(fname.begin(), fname.end());

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  int recipient_fd = context->messaging_service->get_fd_by_user_id(
      context->messaging_service->get_user_by_name(recipient_username_str));

  std::cout << "[File] Server file chunk is sending " << name << std::endl;
  context->transport_server->send(recipient_fd,
                                  context->serializer.serialize(msg));
}

MessageType FileEndHandler::getMessageType() const {
  return MessageType::FileEnd;
}

void FileEndHandler::handleMessageOnClient(
    const Message &msg, const std::shared_ptr<ClientContext> context) {

  pending_files = context->pending_files;
  auto fname = msg.get_meta(1);
  auto recipient = msg.get_meta(0);
  auto name = std::string(fname.begin(), fname.end());

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

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

void FileEndHandler::handleMessageOnServer(
    const Message &msg, const std::shared_ptr<ServerContext> context) {
  auto fname = msg.get_meta(1);
  auto recipient = msg.get_meta(0);
  auto name = std::string(fname.begin(), fname.end());

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  int recipient_fd = context->messaging_service->get_fd_by_user_id(
      context->messaging_service->get_user_by_name(recipient_username_str));

  std::cout << "[File] Completed: " << name << std::endl;
  context->transport_server->send(recipient_fd,
                                  context->serializer.serialize(msg));
}
