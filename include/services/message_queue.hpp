#pragma once
#include "../bytes_hash.hpp"
#include "../network/protocol/parser.hpp"
#include "../network/transport/client.hpp"
#include <atomic>
#include <deque>
#include <memory>
#include <shared_mutex>
#include <stdexcept>

struct OutgoingMessage {
  std::vector<uint8_t> bytes;
};

template <typename T, size_t Size> class LockFreeQueue {
  std::array<T, Size> buffer;
  std::atomic<int> head{0};
  std::atomic<int> tail{0};

public:
  bool push(T item);

  std::optional<T> pop();
};

class MessageQueue {
  std::shared_ptr<IClient> client;
  std::unordered_map<std::vector<uint8_t>,
                     std::unique_ptr<LockFreeQueue<OutgoingMessage, 4096>>>
      lock_free_queues;

  Serializer serializer;
  Parser parser;

  std::shared_mutex map_mutex;

public:
  MessageQueue(std::shared_ptr<IClient> client_) : client(client_) {}

  void push(const std::vector<uint8_t> &recipient_id,
            const std::vector<uint8_t> bytes);
  bool empty(std::vector<uint8_t> &recipient_username);

  std::optional<OutgoingMessage> pop();

  std::optional<OutgoingMessage>
  find_pending(const std::vector<uint8_t> &recipient_id);
};
