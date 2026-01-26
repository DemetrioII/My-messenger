#pragma once
#include "../bytes_hash.hpp"
#include "../network/protocol/parser.hpp"
#include "../network/transport/client.hpp"
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>

struct OutgoingMessage {
  std::vector<uint8_t> recipient_id;
  std::vector<uint8_t> bytes;
};

class MessageQueue {
  std::shared_ptr<IClient> client;

  Serializer serializer;
  Parser parser;

  std::mutex mutex_;
  std::unordered_map<std::vector<uint8_t>, std::deque<OutgoingMessage>> queue_;

public:
  MessageQueue(std::shared_ptr<IClient> client_) : client(client_) {}

  void push(const std::vector<uint8_t> &recipient_id,
            const std::vector<uint8_t> bytes);
  bool empty();

  OutgoingMessage pop();

  std::optional<OutgoingMessage>
  find_pending(const std::vector<uint8_t> &recipient_id);
};
