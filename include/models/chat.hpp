// chat.hpp
#pragma once
#include "message.hpp"
#include <string>
#include <unordered_set>
#include <vector>

typedef enum { Private, Group, Channel } ChatType;

class Chat {
  std::string id;
  std::string name;
  ChatType type;
  std::unordered_set<std::string> members_id;
  std::vector<Message> messages;

public:
  Chat() = default;

  Chat(const std::string &id, const std::string &name, ChatType type)
      : id(id), name(name), type(type) {}

  void addMessage(const Message &msg) { messages.push_back(msg); }

  std::vector<Message> get_recent_message(size_t count = 50) const {
    if (messages.size() <= count)
      return messages;

    return std::vector<Message>(messages.end() - count, messages.end());
  }

  bool add_member(const std::string &user_id) {
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

  bool is_member(const std::string &user_id) {
    return members_id.find(user_id) != members_id.end();
  }

  const std::string get_id() const { return id; }
  const std::string get_name() const { return name; }
  ChatType get_type() const { return type; }
  const std::vector<std::string> get_members() const {
    std::vector<std::string> members;
    for (auto &i : members_id) {
      members.push_back(i);
    }

    return members;
  }

  bool remove_member(const std::string &user_id) {
    if (members_id.find(user_id) == members_id.end()) {
      return false;
    }

    members_id.erase(user_id);
    return true;
  }
};
