#pragma once
#include "../message_dispatcher.hpp"
#include <fstream>

class FileStartHandler : public IMessageHandler {
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

  uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessageType getMessageType() const override;

  void handleMessageOnClient(const Message &msg,
                             std::shared_ptr<ClientContext> context) override;

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override;
};

class FileChunkHandler : public IMessageHandler {
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

  uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessageType getMessageType() const override;

  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override;

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override;
};

class FileEndHandler : public IMessageHandler {
  std::shared_ptr<
      std::unordered_map<std::string, std::unique_ptr<std::ofstream>>>
      pending_files;

  uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessageType getMessageType() const override;

  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override;

  void
  handleMessageOnServer(const Message &msg,
                        const std::shared_ptr<ServerContext> context) override;
};
