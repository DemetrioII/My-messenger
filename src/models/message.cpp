#include "include/models/message.hpp"

#ifdef MESSENGER_WITH_PROTOBUF
#include "messenger.pb.h"
#endif

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
uint64_t current_timestamp_seconds() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

uint32_t compute_serialized_size(uint8_t metalen,
                                 const std::vector<std::vector<uint8_t>> &metadata,
                                 const std::vector<uint8_t> &payload) {
  uint64_t total = 0;
  total += 1; // version
  total += 1; // type
  total += 4; // id
  total += 4; // checksum
  total += 8; // timestamp
  total += 8; // sequence_number
  total += 4; // flags
  total += 1; // metadata count
  for (const auto &m : metadata) {
    total += 4; // protobuf bytes length prefix when encoded
    total += m.size();
  }
  total += 4; // payload length prefix when encoded
  total += payload.size();

  if (metalen != metadata.size()) {
    throw std::runtime_error("Malformed message metadata count");
  }
  if (total > std::numeric_limits<uint32_t>::max()) {
    throw std::overflow_error("Message is too large");
  }
  return static_cast<uint32_t>(total);
}

MessageType to_message_type(uint32_t value) {
  switch (value) {
  case 0x00:
    return MessageType::Text;
  case 0x01:
    return MessageType::Command;
  case 0x02:
    return MessageType::HandShake;
  case 0x03:
    return MessageType::CipherMessage;
  case 0x04:
    return MessageType::Response;
  case 0x05:
    return MessageType::FileStart;
  case 0x06:
    return MessageType::FileChunk;
  case 0x07:
    return MessageType::FileEnd;
  default:
    return MessageType::Text;
  }
}

uint32_t to_u32(MessageType value) {
  return static_cast<uint32_t>(value);
}
} // namespace

Message::Message(const std::vector<uint8_t> &payload_, uint8_t metalen_,
                 const std::vector<std::vector<uint8_t>> &metadata_,
                 MessageType type)
    : header{type, 0, 0, 0, current_timestamp_seconds(), 1},
      metalen(metalen_), envelope{}, metadata(metadata_), payload(payload_) {
  metalen = static_cast<uint8_t>(metadata.size());
  header.length = compute_serialized_size(metalen, metadata, payload);
}

Message::Message(std::vector<uint8_t> &&payload_, uint8_t metalen_,
                 std::vector<std::vector<uint8_t>> &&metadata_,
                 MessageType type)
    : header{type, 0, 0, 0, current_timestamp_seconds(), 1},
      metalen(metalen_), envelope{}, metadata(std::move(metadata_)),
      payload(std::move(payload_)) {
  metalen = static_cast<uint8_t>(metadata.size());
  header.length = compute_serialized_size(metalen, metadata, payload);
}

MessageType Message::get_type() const { return header.type; }

void Message::set_payload(const std::vector<uint8_t> &new_payload) {
  payload = new_payload;
  header.length = compute_serialized_size(metalen, metadata, payload);
}

const std::vector<uint8_t> &Message::get_payload() const { return payload; }

const std::vector<uint8_t> &Message::get_meta(size_t index) const {
  static const std::vector<uint8_t> empty_vector = {};
  if (index < metadata.size())
    return metadata[index];
  return empty_vector;
}

size_t Message::meta_count() const { return metadata.size(); }

void Message::insert_metadata(const std::vector<uint8_t> &meta) {
  metadata.push_back(meta);
  metalen = static_cast<uint8_t>(metadata.size());
  header.length = compute_serialized_size(metalen, metadata, payload);
}

bool Message::validate() const {
  return header.protocol_version == 1 && metalen == metadata.size();
}

void Message::update_checksum() { header.checksum = 0; }

std::vector<uint8_t> Serializer::serialize(const Message &msg) const {
#ifdef MESSENGER_WITH_PROTOBUF
  messenger::v1::WireMessage wire;
  wire.set_version(msg.header.protocol_version);
  wire.set_message_type(to_u32(msg.header.type));
  wire.set_message_id(msg.header.id);
  wire.set_checksum(msg.header.checksum);
  wire.set_timestamp(msg.header.timestamp);

  for (const auto &m : msg.metadata) {
    wire.add_metadata(std::string(reinterpret_cast<const char *>(m.data()),
                                 m.size()));
  }
  wire.set_payload(std::string(
      reinterpret_cast<const char *>(msg.payload.data()), msg.payload.size()));

  auto *env = wire.mutable_envelope();
  env->set_sequence_number(msg.envelope.sequence_number);
  env->set_flags(msg.envelope.flags);
  env->set_sender(std::string(
      reinterpret_cast<const char *>(msg.envelope.sender.data()),
      msg.envelope.sender.size()));
  env->set_recipient(std::string(
      reinterpret_cast<const char *>(msg.envelope.recipient.data()),
      msg.envelope.recipient.size()));

  std::string bytes;
  if (!wire.SerializeToString(&bytes)) {
    throw std::runtime_error("Failed to serialize protobuf message");
  }
  return std::vector<uint8_t>(bytes.begin(), bytes.end());
#else
  (void)msg;
  throw std::runtime_error("protobuf support is disabled");
#endif
}

std::vector<uint8_t> Serializer::serialize(Message &&msg) const {
  return serialize(msg);
}

Message Serializer::deserialize(const std::vector<uint8_t> &raw) const {
#ifdef MESSENGER_WITH_PROTOBUF
  messenger::v1::WireMessage wire;
  if (!wire.ParseFromArray(raw.data(), static_cast<int>(raw.size()))) {
    throw std::runtime_error("Failed to parse protobuf message");
  }
  Message msg;
  msg.header.protocol_version = static_cast<uint8_t>(wire.version());
  msg.header.type = to_message_type(wire.message_type());
  msg.header.id = wire.message_id();
  msg.header.checksum = wire.checksum();
  msg.header.timestamp = wire.timestamp();

  msg.metadata.clear();
  for (const auto &m : wire.metadata()) {
    msg.metadata.emplace_back(m.begin(), m.end());
  }
  msg.metalen = static_cast<uint8_t>(msg.metadata.size());

  if (wire.has_envelope()) {
    const auto &env = wire.envelope();
    msg.envelope.sequence_number = env.sequence_number();
    msg.envelope.flags = env.flags();
    msg.envelope.sender.assign(env.sender().begin(), env.sender().end());
    msg.envelope.recipient.assign(env.recipient().begin(), env.recipient().end());
  }

  const auto &payload = wire.payload();
  msg.payload.assign(payload.begin(), payload.end());
  msg.header.length =
      compute_serialized_size(msg.metalen, msg.metadata, msg.payload);
  return msg;
#else
  (void)raw;
  throw std::runtime_error("protobuf support is disabled");
#endif
}

Message Serializer::deserialize(std::vector<uint8_t> &&raw) const {
  return deserialize(raw);
}
