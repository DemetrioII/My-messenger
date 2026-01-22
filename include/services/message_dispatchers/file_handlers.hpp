#pragma once
#include "../message_dispatcher.hpp"
#include <fstream>

uint64_t from_bytes(const std::vector<uint8_t> &v) {
  uint64_t r = 0;
  for (int i = 0; i < 8; i++)
    r |= (uint64_t)(v[i]) << (i * 8);
  return r;
}

class FileStartHandler : public IMessageHandler {
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

public:
  MessageType getMessageType() const override { return MessageType::FileStart; }

  void handleMessageOnClient(const Message &msg,
                             std::shared_ptr<ClientContext> context) override {
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
      (*pending_files)[recipient_username_str + " " + name] =
          std::move(file_ptr);
    }
  }

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override {
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
};

class FileChunkHandler : public IMessageHandler {
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

public:
  MessageType getMessageType() const override { return MessageType::FileChunk; }

  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override {
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

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override {
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
};

class FileEndHandler : public IMessageHandler {
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

public:
  MessageType getMessageType() const override { return MessageType::FileEnd; }

  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override {

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

  void
  handleMessageOnServer(const Message &msg,
                        const std::shared_ptr<ServerContext> context) override {
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
};
