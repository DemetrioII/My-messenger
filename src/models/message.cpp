#include "include/models/message.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
constexpr uint8_t kProtocolVersion = 1;
constexpr uint8_t kExtendedFormatFlag = 0x80;

uint64_t current_timestamp_seconds() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

uint32_t compute_serialized_size(uint8_t metalen,
                                 const std::vector<std::vector<uint8_t>> &metadata,
                                 const std::vector<uint8_t> &payload) {
  uint64_t total = 2; // type + metadata count
  for (const auto &m : metadata) {
    total += 2; // metadata length
    total += m.size();
  }
  total += 4; // payload length field
  total += payload.size();

  if (metalen != metadata.size()) {
    throw std::runtime_error("Malformed message metadata count");
  }

  if (total > std::numeric_limits<uint32_t>::max()) {
    throw std::overflow_error("Message is too large");
  }
  return static_cast<uint32_t>(total);
}

uint32_t read_u32(const std::vector<uint8_t> &raw, size_t off) {
  return (static_cast<uint32_t>(raw[off]) << 24) |
         (static_cast<uint32_t>(raw[off + 1]) << 16) |
         (static_cast<uint32_t>(raw[off + 2]) << 8) |
         static_cast<uint32_t>(raw[off + 3]);
}

uint64_t read_u64(const std::vector<uint8_t> &raw, size_t off) {
  uint64_t value = 0;
  for (size_t i = 0; i < 8; ++i) {
    value |= static_cast<uint64_t>(raw[off + i]) << (i * 8);
  }
  return value;
}

void write_u32(std::vector<uint8_t> &out, uint32_t value) {
  out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(value & 0xFF));
}

void write_u64(std::vector<uint8_t> &out, uint64_t value) {
  for (size_t i = 0; i < 8; ++i) {
    out.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
  }
}
} // namespace

Message::Message(const std::vector<uint8_t> &payload_, uint8_t metalen_,
                 const std::vector<std::vector<uint8_t>> &metadata_,
                 MessageType type)
    : header{type,
             0,
             0,
             current_timestamp_seconds(),
             kProtocolVersion},
      metalen(metalen_), envelope{}, metadata(metadata_), payload(payload_) {
  metalen = static_cast<uint8_t>(metadata.size());
  header.length = compute_serialized_size(metalen, metadata, payload);
}

Message::Message(std::vector<uint8_t> &&payload_, uint8_t metalen_,
                 std::vector<std::vector<uint8_t>> &&metadata_,
                 MessageType type)
    : header{type,
             0,
             0,
             current_timestamp_seconds(),
             kProtocolVersion},
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
  return header.protocol_version == kProtocolVersion &&
         metalen == metadata.size();
}

void Message::update_checksum() {
  // The transport already authenticates payloads; keep checksum reserved for
  // future wire-format compatibility.
  header.checksum = 0;
}

std::vector<uint8_t> Serializer::serialize(const Message &msg) const {
  std::vector<uint8_t> out;
  const bool use_extended_format =
      !msg.envelope.sender.empty() || !msg.envelope.recipient.empty() ||
      msg.envelope.sequence_number != 0 || msg.envelope.flags != 0 ||
      msg.header.protocol_version != kProtocolVersion;

  if (!use_extended_format) {
    out.reserve(msg.header.length);
    out.push_back(static_cast<uint8_t>(msg.header.type));
    out.push_back(msg.metalen);

    for (const auto &m : msg.metadata) {
      uint16_t len = static_cast<uint16_t>(m.size());
      out.push_back(static_cast<uint8_t>(len >> 8));
      out.push_back(static_cast<uint8_t>(len & 0xFF));
      out.insert(out.end(), m.begin(), m.end());
    }

    uint32_t plen = static_cast<uint32_t>(msg.payload.size());
    write_u32(out, plen);
    out.insert(out.end(), msg.payload.begin(), msg.payload.end());
    return out;
  }

  out.reserve(2 + 4 + 8 + 1 + 1 + 1 + msg.envelope.sender.size() +
              msg.envelope.recipient.size() + msg.metadata.size() * 2 +
              msg.payload.size());

  out.push_back(static_cast<uint8_t>(msg.header.type));
  out.push_back(static_cast<uint8_t>(kExtendedFormatFlag |
                                     (msg.header.protocol_version & 0x7F)));
  write_u32(out, msg.envelope.flags);
  write_u64(out, msg.header.timestamp);

  out.push_back(static_cast<uint8_t>(msg.envelope.sender.size()));
  out.insert(out.end(), msg.envelope.sender.begin(), msg.envelope.sender.end());
  out.push_back(static_cast<uint8_t>(msg.envelope.recipient.size()));
  out.insert(out.end(), msg.envelope.recipient.begin(),
             msg.envelope.recipient.end());
  write_u64(out, msg.envelope.sequence_number);
  write_u32(out, msg.envelope.flags);

  out.push_back(msg.metalen);
  for (const auto &m : msg.metadata) {
    uint16_t len = static_cast<uint16_t>(m.size());
    out.push_back(static_cast<uint8_t>(len >> 8));
    out.push_back(static_cast<uint8_t>(len & 0xFF));
    out.insert(out.end(), m.begin(), m.end());
  }

  write_u32(out, static_cast<uint32_t>(msg.payload.size()));
  out.insert(out.end(), msg.payload.begin(), msg.payload.end());

  return out;
}

std::vector<uint8_t> Serializer::serialize(Message &&msg) const {
  return serialize(msg);
}

Message Serializer::deserialize(const std::vector<uint8_t> &raw) const {
  if (raw.size() < 6) {
    throw std::runtime_error("Malformed message: too short");
  }

  Message msg;
  size_t off = 0;

  msg.header.type = static_cast<MessageType>(raw[off++]);

  const uint8_t second = raw[off++];
  const bool extended = (second & kExtendedFormatFlag) != 0;

  if (!extended) {
    msg.header.protocol_version = kProtocolVersion;
    msg.metalen = second;
    msg.metadata.reserve(msg.metalen);

    for (size_t i = 0; i < msg.metalen; ++i) {
      if (off + 2 > raw.size()) {
        throw std::runtime_error("Malformed message: metadata length missing");
      }
      uint16_t len = (static_cast<uint16_t>(raw[off]) << 8) |
                     static_cast<uint16_t>(raw[off + 1]);
      off += 2;
      if (off + len > raw.size()) {
        throw std::runtime_error("Malformed message: metadata out of bounds");
      }
      msg.metadata.emplace_back(raw.begin() + off, raw.begin() + off + len);
      off += len;
    }

    if (off + 4 > raw.size()) {
      throw std::runtime_error("Malformed message: payload length missing");
    }

    uint32_t plen = read_u32(raw, off);
    off += 4;

    if (off + plen > raw.size()) {
      throw std::runtime_error("Malformed message: payload out of bounds");
    }

    msg.payload.insert(msg.payload.end(), raw.begin() + off,
                       raw.begin() + off + plen);
    msg.header.length =
        compute_serialized_size(msg.metalen, msg.metadata, msg.payload);
    return msg;
  }

  msg.header.protocol_version = second & 0x7F;
  if (msg.header.protocol_version == 0) {
    throw std::runtime_error("Unsupported protocol version");
  }

  if (off + 4 > raw.size()) {
    throw std::runtime_error("Malformed message: flags missing");
  }
  msg.envelope.flags = read_u32(raw, off);
  off += 4;

  if (off + 8 > raw.size()) {
    throw std::runtime_error("Malformed message: timestamp missing");
  }
  msg.header.timestamp = read_u64(raw, off);
  off += 8;

  if (off >= raw.size()) {
    throw std::runtime_error("Malformed message: sender length missing");
  }
  const size_t sender_len = raw[off++];
  if (off + sender_len > raw.size()) {
    throw std::runtime_error("Malformed message: sender out of bounds");
  }
  msg.envelope.sender.assign(raw.begin() + off, raw.begin() + off + sender_len);
  off += sender_len;

  if (off >= raw.size()) {
    throw std::runtime_error("Malformed message: recipient length missing");
  }
  const size_t recipient_len = raw[off++];
  if (off + recipient_len > raw.size()) {
    throw std::runtime_error("Malformed message: recipient out of bounds");
  }
  msg.envelope.recipient.assign(raw.begin() + off,
                                raw.begin() + off + recipient_len);
  off += recipient_len;

  if (off + 8 > raw.size()) {
    throw std::runtime_error("Malformed message: sequence missing");
  }
  msg.envelope.sequence_number = read_u64(raw, off);
  off += 8;

  if (off + 4 > raw.size()) {
    throw std::runtime_error("Malformed message: envelope flags missing");
  }
  msg.envelope.flags = read_u32(raw, off);
  off += 4;

  if (off >= raw.size()) {
    throw std::runtime_error("Malformed message: metadata count missing");
  }
  msg.metalen = raw[off++];
  msg.metadata.reserve(msg.metalen);

  for (size_t i = 0; i < msg.metalen; ++i) {
    if (off + 2 > raw.size()) {
      throw std::runtime_error("Malformed message: metadata length missing");
    }
    uint16_t len = (static_cast<uint16_t>(raw[off]) << 8) |
                   static_cast<uint16_t>(raw[off + 1]);
    off += 2;
    if (off + len > raw.size()) {
      throw std::runtime_error("Malformed message: metadata out of bounds");
    }
    msg.metadata.emplace_back(raw.begin() + off, raw.begin() + off + len);
    off += len;
  }

  if (off + 4 > raw.size()) {
    throw std::runtime_error("Malformed message: payload length missing");
  }
  const uint32_t plen = read_u32(raw, off);
  off += 4;
  if (off + plen > raw.size()) {
    throw std::runtime_error("Malformed message: payload out of bounds");
  }
  msg.payload.insert(msg.payload.end(), raw.begin() + off,
                     raw.begin() + off + plen);
  msg.header.length = static_cast<uint32_t>(raw.size() - 2);
  return msg;
}

Message Serializer::deserialize(std::vector<uint8_t> &&raw) const {
  Message msg = deserialize(raw);
  return msg;
}
