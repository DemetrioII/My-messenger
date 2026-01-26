#include "../../../include/services/message_queue.hpp"

void MessageQueue::push(const std::vector<uint8_t> &recipient_id,
                        const std::vector<uint8_t> bytes) {
  std::lock_guard<std::mutex> lock(mutex_);
  OutgoingMessage msg{.recipient_id = recipient_id, .bytes = bytes};
  queue_[recipient_id].push_back(msg);
}

bool MessageQueue::empty() {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.empty();
}

OutgoingMessage MessageQueue::pop() {
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
MessageQueue::find_pending(const std::vector<uint8_t> &recipient_id) {
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
