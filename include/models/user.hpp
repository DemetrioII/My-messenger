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
  std::vector<uint8_t> publicDHKey;
  std::vector<uint8_t> publicIdentityKey;
  std::vector<uint8_t> signature;
  UserStatus status;
  std::vector<std::string> chat_ids;

public:
  User() = default;

  User(const std::string &id, const std::string &username);

  void set_status(UserStatus new_status);
  UserStatus get_status() const;

  void join_chat(const std::string &chat_id);

  bool has_chat(const std::string &chat_id) const;

  const std::string &get_id() const;

  const std::string &get_username() const;

  void set_key_signature(const std::vector<uint8_t> &bytes);

  void set_public_DH_key(const std::vector<uint8_t> &pub_bytes);

  void set_public_Identity_key(const std::vector<uint8_t> &pub_bytes);

  std::vector<uint8_t> get_public_DH_key();

  std::vector<uint8_t> get_key_signature();

  std::vector<uint8_t> get_public_Identity_key();

  bool verify_signature(const std::string &data,
                        const std::string &signature) const;
};
