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

  Chat(const std::string &id, const std::string &name, ChatType type);

  void addMessage(const Message &msg);

  std::vector<Message> get_recent_message(size_t count = 50) const;

  bool add_member(const std::string &user_id);

  bool is_member(const std::string &user_id);

  const std::string get_id() const;
  const std::string get_name() const;
  ChatType get_type() const;
  const std::vector<std::string> get_members() const;

  bool remove_member(const std::string &user_id);
};
