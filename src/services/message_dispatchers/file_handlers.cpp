#include "../../../include/services/message_dispatchers/file_handlers.hpp"

MessageType FileStartHandler::getMessageType() const {
  return MessageType::FileStart;
}

void FileStartHandler::handleMessageOnClient(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  pending_files = context->pending_files;
  // auto recipient = msg.get_meta(0);
  // auto fname = msg.get_meta(1);
  // auto size_bytes = msg.get_meta(2);
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

void FileStartHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  // auto recipient = msg.get_meta(0);
  // auto fname = msg.get_meta(1);
  // auto size_bytes = msg.get_meta(2);
  uint64_t file_size = from_bytes(msg.get_meta(2));

  std::string_view name(reinterpret_cast<const char *>(msg.get_meta(1).data()),
                        msg.get_meta(1).size());
  std::cout << "[File] Server Receiving " << name << " (" << file_size
            << " bytes)" << std::endl;

  std::string recipient_username_str =
      std::string(msg.get_meta(0).begin(), msg.get_meta(0).end());

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
  // auto recipient = msg.get_meta(0);
  // auto fname = msg.get_meta(1);
  // auto sender = msg.get_meta(2);
  auto name = std::string(msg.get_meta(1).begin(), msg.get_meta(1).end());

  std::string recipient_username_str(msg.get_meta(0).begin(),
                                     msg.get_meta(0).end());

  if (pending_files->find(recipient_username_str + " " + name) !=
      pending_files->end()) {

    // auto payload = msg.get_payload();
    // std::vector<uint8_t> cipher_bytes(msg.get_payload().begin(),
    // msg.get_payload().end());
    std::vector<uint8_t> payload = context->encryption_service->decrypt_for(
        msg.get_meta(2), msg.get_meta(0), msg.get_payload());
    (*pending_files)[recipient_username_str + " " + name]->write(
        reinterpret_cast<const char *>(payload.data()), payload.size());
  }

  std::cout << "[File] File chunk has been received " << name << std::endl;
}

void FileChunkHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  // auto recipient = msg.get_meta(0);
  // auto fname = msg.get_meta(1);
  std::string_view name(reinterpret_cast<const char *>(msg.get_meta(1).data()),
                        msg.get_meta(1).size());

  std::string recipient_username_str =
      std::string(msg.get_meta(0).begin(), msg.get_meta(0).end());

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

void FileEndHandler::handleMessageOnServer(
    const Message &msg, const std::shared_ptr<ServerContext> context) {
  // auto fname = msg.get_meta(1);
  // auto recipient = msg.get_meta(0);
  auto name =
      std::string_view(reinterpret_cast<const char *>(msg.get_meta(1).data()),
                       msg.get_meta(1).size());

  std::string recipient_username_str =
      std::string(msg.get_meta(0).begin(), msg.get_meta(0).end());

  int recipient_fd = context->messaging_service->get_fd_by_user_id(
      context->messaging_service->get_user_by_name(recipient_username_str));

  std::cout << "[File] Completed: " << name << std::endl;
  context->transport_server->send(recipient_fd,
                                  context->serializer.serialize(msg));
}
