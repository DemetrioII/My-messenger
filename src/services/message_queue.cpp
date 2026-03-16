#include "../../../include/services/message_queue.hpp"

template <typename T, size_t Size> bool LockFreeQueue<T, Size>::push(T item) {
  size_t t = tail.load(std::memory_order_relaxed);
  size_t t_next = (t + 1) % Size;
  if (t_next == head.load(std::memory_order_acquire)) {
    return false;
  }
  buffer[t] = std::move(item);
  tail.store(t_next, std::memory_order_release);
  return true;
}

template <typename T, size_t Size>
std::optional<T> LockFreeQueue<T, Size>::pop() {
  size_t h = head.load(std::memory_order_relaxed);
  if (h == tail.load(std::memory_order_acquire)) {
    return std::nullopt;
  }
  T item = std::move(buffer[h]);
  head.store((h + 1) % Size, std::memory_order_release);
  return item;
}

void MessageQueue::push(const std::vector<uint8_t> &recipient_username,
                        const std::vector<uint8_t> bytes) {
  std::lock_guard<std::shared_mutex> lock(map_mutex);
  if (lock_free_queues.find(recipient_username) == lock_free_queues.end()) {
    lock_free_queues[recipient_username] =
        std::make_unique<LockFreeQueue<OutgoingMessage, 4096>>();
  }
  lock_free_queues[recipient_username]->push({.bytes = bytes});
}

bool MessageQueue::empty(std::vector<uint8_t> &recipient_username) {
  std::lock_guard<std::shared_mutex> lock(map_mutex);
  return false;
}

std::optional<OutgoingMessage> MessageQueue::pop() {

  throw std::runtime_error("MessageQueue is empty");
}

std::optional<OutgoingMessage>
MessageQueue::find_pending(const std::vector<uint8_t> &recipient_username) {
  std::lock_guard<std::shared_mutex> lock(map_mutex);
  auto it = lock_free_queues.find(recipient_username);
  if (it != lock_free_queues.end()) {
    auto msg = it->second->pop();
    return msg;
  }
  return std::nullopt;
}
