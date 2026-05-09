#pragma once

#include "include/application/dispatching/message_handler_interfaces.hpp"
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

class ClientFileStartHandler : public IClientMessageHandler {
  uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessageType getMessageType() const override;

  void handleIncoming(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  void handleOutgoing(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;
};

class ClientFileChunkHandler : public IClientMessageHandler {
  uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessageType getMessageType() const override;

  void handleIncoming(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  void handleOutgoing(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;
};

class ClientFileEndHandler : public IClientMessageHandler {
  uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessageType getMessageType() const override;

  void handleIncoming(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  void handleOutgoing(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;
};
