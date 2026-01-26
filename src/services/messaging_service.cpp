#include "../../../include/services/messageing_service.hpp"

std::string
MessagingService::authenticate(const std::string &username,
                               const std::string &password_hash,
                               const std::vector<uint8_t> &pubkey_bytes) {
  auto it = std::find_if(users.begin(), users.end(),
                         [&](const std::pair<std::string, User> &p) {
                           return p.second.get_username() == username;
                         });

  if (it != users.end()) {
    return "";
  }

  User user;
  std::string token = generate_token();

  user = User("user_" + std::to_string(users.size()), username);
  auth_tokens[token] = user.get_id();
  users[user.get_id()] = user;

  user.set_public_key(pubkey_bytes);

  return token;
}

std::string MessagingService::get_user_id_by_token(std::string &token) {
  auto it = auth_tokens.find(token);
  if (it != auth_tokens.end())
    return it->second;
  return "";
}

void MessagingService::remove_user_by_fd(int fd) {
  if (!is_authenticated(fd)) {
    return;
  }

  auto user_id = connections[fd];
  connections.erase(fd);
  fds.erase(user_id);
  auth_tokens.erase(user_id);

  for (auto &chat : chats) {
    if (chat.second.is_member(user_id)) {
      chat.second.remove_member(user_id);
    }
  }
  users.erase(user_id);
}

void MessagingService::bind_connection(int fd, const std::string &user_id) {
  connections[fd] = user_id;
  fds[user_id] = fd;
}

bool MessagingService::is_authenticated(int fd) {
  return connections.find(fd) != connections.end();
}

int MessagingService::get_fd_by_user_id(const std::string &user_id) {
  return fds[user_id];
}

std::string MessagingService::chat_id_by_name(std::string &chat_name) {
  for (auto &chat : chats) {
    if (chat.second.get_name() == chat_name)
      return chat.second.get_id();
  }
  return "";
}

bool MessagingService::send_message(const std::string &from_user_id,
                                    const std::string &to_chat_id,
                                    const Message &message) {
  auto it = chats.find(to_chat_id);
  if (it == chats.end())
    return false;

  Chat &chat = it->second;
  if (!chat.is_member(from_user_id))
    return false;

  chat.addMessage(message);
  message_sent.emit_subsriber_event({from_user_id, to_chat_id, message});
  std::cout << "Пользователь (ID: " << from_user_id
            << ") отправил сообщение в чат " << "(ID: " << to_chat_id << ")"
            << std::endl;
  return true;
}

bool MessagingService::join_chat_by_name(const std::string &user_id,
                                         const std::string &chat_name) {
  for (auto &[id, chat] : chats) {
    if (chat.get_name() == chat_name) {
      chat.add_member(user_id);
      users[user_id].join_chat(id);

      return true;
    }
  }
  return false;
}

User &MessagingService::get_user_by_id(std::string user_id) {
  return users[user_id];
}

std::string MessagingService::get_user_by_name(std::string username) {
  for (auto &u : users) {
    if (u.second.get_username() == username)
      return u.first;
  }
  return "";
}

std::string MessagingService::get_user_id_by_fd(int fd) {
  if (connections.find(fd) != connections.end())
    return connections[fd];
  else
    return "";
}

std::string MessagingService::generate_chat_id() {
  // ВРЕМЕННО
  return "chat_" + std::to_string(chats.size() + 1);
}

Chat &MessagingService::get_chat_by_id(const std::string &chat_id) {
  return chats[chat_id];
}

std::string
MessagingService::get_chat_id_by_name(const std::string &chat_name) {
  for (auto &[chat_id, chat_obj] : chats) {
    if (chat_obj.get_name() == chat_name) {
      return chat_id;
    }
  }
  return "";
}

bool MessagingService::is_member_of_chat(const std::string &chat_name,
                                         const std::string &user_id) {
  Chat &chat = get_chat_by_id(get_chat_id_by_name(chat_name));
  return chat.is_member(user_id);
}

std::string
MessagingService::create_chat(const std::string &creator_id,
                              const std::string &name, ChatType type,
                              const std::vector<std::string> &initial_members) {
  auto chat_id = generate_chat_id();
  Chat chat(chat_id, name, type);
  for (auto &i : initial_members) {
    chat.add_member(i);
    auto user = users.find(i);
    if (user != users.end())
      user->second.join_chat(chat_id);
  }

  chats[chat_id] = std::move(chat);

  return chat_id;
}

std::string MessagingService::generate_token() {
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<uint64_t> dis;

  std::stringstream ss;
  ss << std::hex << dis(gen) << dis(gen);
  return ss.str();
}
