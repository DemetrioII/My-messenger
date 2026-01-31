#include "../../include/models/message.hpp"

Message::Message(const std::vector<uint8_t> &payload_, uint8_t metalen_,
                 const std::vector<std::vector<uint8_t>> &metadata_,
                 MessageType type) {
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

MessageType Message::get_type() const { return header.type; }

void Message::set_payload(const std::vector<uint8_t> &new_payload) {
  payload = new_payload;
}

const std::vector<uint8_t> &Message::get_payload() const { return payload; }

const std::vector<uint8_t> &Message::get_meta(size_t index) const {
  static const std::vector<uint8_t> empty_vector = {};
  if (index < metalen)
    return metadata[index];
  return empty_vector;
}

void Message::insert_metadata(const std::vector<uint8_t> &meta) {
  metadata.push_back(meta);
}

std::vector<uint8_t> Serializer::serialize(const Message &msg) const {
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

Message Serializer::deserialize(const std::vector<uint8_t> &raw) const {
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
