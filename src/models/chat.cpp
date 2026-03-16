#include "../../include/models/chat.hpp"

Chat::Chat(const std::string &id, const std::string &name, ChatType type)
    : id(id), name(name), type(type) {}

void Chat::addMessage(const Message &msg) { messages.push_back(msg); }

std::vector<Message> Chat::get_recent_message(size_t count) const {
  if (messages.size() <= count)
    return messages;

  return std::vector<Message>(messages.end() - count, messages.end());
}

bool Chat::add_member(const std::string &user_id) {
  if (type == Private) {
    return false;
  }

  if (members_id.find(user_id) != members_id.end())
    return false;

  members_id.insert(user_id);
  return true;
  // Мы не добавляем в пользовательские чаты идентификатор этого чата,
  // поскольку чат не управляет пользователем
}

bool Chat::is_member(const std::string &user_id) {
  return members_id.find(user_id) != members_id.end();
}

const std::string Chat::get_id() const { return id; }
const std::string Chat::get_name() const { return name; }
ChatType Chat::get_type() const { return type; }
const std::vector<std::string> Chat::get_members() const {
  std::vector<std::string> members;
  for (auto &i : members_id) {
    members.push_back(i);
  }

  return members;
}

bool Chat::remove_member(const std::string &user_id) {
  if (members_id.find(user_id) == members_id.end()) {
    return false;
  }

  members_id.erase(user_id);
  return true;
}
