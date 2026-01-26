#include "../../include/models/user.hpp"

User::User(const std::string &id, const std::string &username)
    : id(id), username(username), status(UserStatus::Offline) {}

void User::set_status(UserStatus new_status) { status = new_status; }
UserStatus User::get_status() const { return status; }

void User::join_chat(const std::string &chat_id) {
  chat_ids.push_back(chat_id);
}

bool User::has_chat(const std::string &chat_id) const {
  return std::find(chat_ids.begin(), chat_ids.end(), chat_id) != chat_ids.end();
}

const std::string &User::get_id() const { return id; }

const std::string &User::get_username() const { return username; }

void User::set_public_key(const std::vector<uint8_t> &pub_bytes) {
  publicKey.insert(publicKey.end(), pub_bytes.begin(), pub_bytes.end());
}

std::vector<uint8_t> User::get_public_key() { return publicKey; }

bool User::verify_signature(const std::string &data,
                            const std::string &signature) const {
  return true;
}
