#pragma once
#include "../message_dispatcher.hpp"
#include <fstream>

struct FileKey {
  std::string recipient;
  std::string filename;

  bool operator==(const FileKey &other) const {
    return recipient == other.recipient && filename == other.filename;
  }
};

struct FileKeyHash {
  std::size_t operator()(const FileKey &k) const {
    return std::hash<std::string>()(k.recipient) ^
           (std::hash<std::string>()(k.filename) << 1);
  }
};

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

  void handleOnSendPeer(const Message &msg,
                        std::shared_ptr<PeerContext> context) override;

  void handleOnRecvPeer(const Message &msg,
                        std::shared_ptr<PeerContext> context) override;
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

  void handleOnSendPeer(const Message &msg,
                        std::shared_ptr<PeerContext> context) override;

  void handleOnRecvPeer(const Message &msg,
                        std::shared_ptr<PeerContext> context) override;
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

  void handleOnSendPeer(const Message &msg,
                        std::shared_ptr<PeerContext> context) override;

  void handleOnRecvPeer(const Message &msg,
                        std::shared_ptr<PeerContext> context) override;
};
