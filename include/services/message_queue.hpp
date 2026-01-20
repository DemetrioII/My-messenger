#pragma once
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
            const std::vector<uint8_t> bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    OutgoingMessage msg{.recipient_id = recipient_id, .bytes = bytes};
    queue_[recipient_id].push_back(msg);
  }

  bool empty() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  OutgoingMessage pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      throw std::runtime_error("MessageQueue is empty");
    }

    for (auto &[recipient, messages] : queue_) {
      if (!messages.empty()) {
        auto msg = messages.front();
        messages.pop_front();
        if (messages.empty()) {
          queue_.erase(recipient);
        }
        return msg;
      }
    }

    throw std::runtime_error("MessageQueue is empty");
  }

  std::optional<OutgoingMessage>
  find_pending(const std::vector<uint8_t> &recipient_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = queue_.find(recipient_id);
    if (it != queue_.end() && !it->second.empty()) {
      auto msg = it->second.front();
      it->second.pop_front();
      if (it->second.empty()) {
        queue_.erase(it);
      }
      return msg;
    }
    return std::nullopt;
  }
};
