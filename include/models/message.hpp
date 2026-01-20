// message.hpp
#pragma once
#include <any>
#include <chrono>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace std {
template <> struct hash<std::vector<uint8_t>> {
  size_t operator()(const std::vector<uint8_t> &v) const noexcept {
    size_t h = 0xcbf29ce484222325;
    for (auto b : v) {
      h ^= b;
      h *= 0x100000001b3;
    }
    return h;
  }
};
} // namespace std

enum class MessageType {
  Text = 0x00,
  Command = 0x01,
  HandShake = 0x02,
  CipherMessage = 0x03,
  Response = 0x04,
  FileStart = 0x05,
  FileChunk = 0x06,
  FileEnd = 0x07
};

enum class CommandType {
  SEND = 0x01,
  LOGIN = 0x02,
  MAKE_ROOM = 0x03,
  JOIN = 0x04,
  GET_PUBKEY = 0x05,
  GET_USERNAME = 0x06,
  GET_ID = 0x07,
  PRIVATE_MESSAGE = 0x08,
  SEND_FILE = 0x09,
  HELP = 0x10,
  EXIT = 0xFE,
  UNKNOWN = 0xFF,
};

struct MessageHeader {
  MessageType type;
  uint32_t length;
  uint32_t checksum;
  uint64_t timestamp;

  std::vector<uint8_t> serialize() const;
  static MessageHeader deserialize(const std::vector<uint8_t> &data);

  // bool validate() const;
};

class Message {
  friend class Serializer;
  friend class Parser;

private:
  MessageHeader header;
  uint8_t metalen;
  std::vector<std::vector<uint8_t>> metadata;
  std::vector<uint8_t> payload;

public:
  Message() = default;
  Message(const std::vector<uint8_t> &payload_, uint8_t metalen_,
          const std::vector<std::vector<uint8_t>> &metadata_,
          MessageType type = MessageType::Text) {
    header.type = type;
    header.length = payload_.size() + metalen_;
    header.timestamp = std::chrono::time_point_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now())
                           .time_since_epoch()
                           .count();
    metalen = metalen_;
    metadata = metadata_;
    payload = payload_;
  }

  MessageType get_type() const { return header.type; }

  void set_payload(const std::vector<uint8_t> &new_payload) {
    payload = new_payload;
  }

  std::vector<uint8_t> get_payload() const { return payload; }

  std::vector<uint8_t> get_meta(size_t index) const {
    if (index < metalen)
      return metadata[index];
    return {};
  }

  void insert_metadata(const std::vector<uint8_t> &meta) {
    metadata.push_back(meta);
  }

  // std::vector<uint8_t> serialize() const;
  // static Message deserialize(const std::vector<uint8_t>& data);

  bool validate() const;
  void update_checksum();
};

class Serializer {
public:
  Serializer() = default;

  std::vector<uint8_t> serialize(const Message &msg) const {
    std::vector<uint8_t> out;

    // type
    out.push_back(static_cast<uint8_t>(msg.header.type));

    // metadata count
    out.push_back(msg.metalen);

    // metadata
    for (auto &m : msg.metadata) {
      uint16_t len = m.size();
      out.push_back(len >> 8);
      out.push_back(len & 0xFF);
      out.insert(out.end(), m.begin(), m.end());
    }

    // payload
    uint32_t plen = msg.payload.size();
    out.push_back((plen >> 24) & 0xFF);
    out.push_back((plen >> 16) & 0xFF);
    out.push_back((plen >> 8) & 0xFF);
    out.push_back(plen & 0xFF);
    out.insert(out.end(), msg.payload.begin(), msg.payload.end());

    return out;
  }

  Message deserialize(const std::vector<uint8_t> &raw) const {
    Message msg;
    size_t off = 0;

    // type
    msg.header.type = static_cast<MessageType>(raw[off++]);

    // meta count
    msg.metalen = raw[off++];
    msg.metadata.reserve(msg.metalen);

    // metadata
    for (int i = 0; i < msg.metalen; ++i) {
      uint16_t len = (raw[off] << 8) | raw[off + 1];
      off += 2;
      msg.metadata.emplace_back(raw.begin() + off, raw.begin() + off + len);
      off += len;
    }

    // payload length
    uint32_t plen = (raw[off] << 24) | (raw[off + 1] << 16) |
                    (raw[off + 2] << 8) | raw[off + 3];
    off += 4;

    // payload
    if (off < raw.size())
      msg.payload.insert(msg.payload.end(), raw.begin() + off,
                         raw.begin() + off + plen);

    return msg;
  }

  ~Serializer() = default;
};

struct ClientContext;

struct ServerContext;

class IMessageHandler {
public:
  virtual ~IMessageHandler() = default;

  virtual void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) = 0;

  virtual void
  handleMessageOnServer(const Message &msg,
                        std::shared_ptr<ServerContext> context) = 0;

  virtual MessageType getMessageType() const = 0;
};
