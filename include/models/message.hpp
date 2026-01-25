// message.hpp
#pragma once
#include <any>
#include <chrono>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

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
          MessageType type = MessageType::Text);

  MessageType get_type() const;

  void set_payload(const std::vector<uint8_t> &new_payload);

  std::vector<uint8_t> get_payload() const;

  std::vector<uint8_t> get_meta(size_t index) const;

  void insert_metadata(const std::vector<uint8_t> &meta);
  // std::vector<uint8_t> serialize() const;
  // static Message deserialize(const std::vector<uint8_t>& data);

  bool validate() const;
  void update_checksum();
};

class Serializer {
public:
  Serializer() = default;

  std::vector<uint8_t> serialize(const Message &msg) const;
  Message deserialize(const std::vector<uint8_t> &raw) const;
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
