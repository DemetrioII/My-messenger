// user.hpp
#pragma once
#include <algorithm>
#include <stdint.h>
#include <string>
#include <vector>

typedef enum { Online, Away, Offline } UserStatus;

class User {
  std::string id;
  std::string username;
  std::vector<uint8_t> publicKey;
  UserStatus status;
  std::vector<std::string> chat_ids;

public:
  User() = default;

  User(const std::string &id, const std::string &username)
      : id(id), username(username), status(UserStatus::Offline) {}

  void set_status(UserStatus new_status) { status = new_status; }
  UserStatus get_status() const { return status; }

  void join_chat(const std::string &chat_id) { chat_ids.push_back(chat_id); }

  bool has_chat(const std::string &chat_id) const {
    return std::find(chat_ids.begin(), chat_ids.end(), chat_id) !=
           chat_ids.end();
  }

  const std::string &get_id() const { return id; }

  const std::string &get_username() const { return username; }

  void set_public_key(const std::vector<uint8_t> &pub_bytes) {
    publicKey.insert(publicKey.end(), pub_bytes.begin(), pub_bytes.end());
  }

  std::vector<uint8_t> get_public_key() { return publicKey; }

  bool verify_signature(const std::string &data,
                        const std::string &signature) const {
    return true;
  }
};
