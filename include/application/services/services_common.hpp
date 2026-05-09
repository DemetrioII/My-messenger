#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T> class Stream {
public:
  using Predicate = std::function<bool(const T &)>;
  using Handler = std::function<void(const T &)>;

  void subscribe(Predicate when, Handler then) {
    subscribers.push_back({std::move(when), std::move(then)});
  }

  void emit_subsriber_event(const T &value) {
    for (auto &s : subscribers)
      if (s.when(value))
        s.then(value);
  }

private:
  struct Subscriber {
    Predicate when;
    Handler then;
  };

  std::vector<Subscriber> subscribers;
};

enum class ServiceError {
  UserNotFound,
  ChatNotFound,
  AccessDenied,
  InvalidToken,
  AlreadyExists,
  AlreadyMember
};

template <typename T> using ServiceResult = std::expected<T, ServiceError>;

using UserIdentifier = std::string;
