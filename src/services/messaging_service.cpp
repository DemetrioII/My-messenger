#include "../../../include/services/messageing_service.hpp"

/*std::string
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

std::expected<bool, ServiceError>
MessagingService::join_chat_by_name(const std::string &user_id,
                                    const std::string &chat_name) {
  /* for (auto &[id, chat] : chats) {
    if (chat.get_name() == chat_name) {
      chat.add_member(user_id);
      users[user_id].join_chat(id);

      return true;
    }
  }
  return false;
  auto id = chat_name_to_id.find(chat_name);
  if (id != chat_name_to_id.end()) {
    return std::unexpected(ServiceError::ChatNotFound);
  }
  auto &chat = chats[id->second];
  if (users.find(user_id) == users.end())
    return std::unexpected(ServiceError::UserNotFound);
  if (!chat.add_member(user_id))
    return std::unexpected(ServiceError::AlreadyMember);
  users[user_id].join_chat(id->second);
  return true;
}

std::expected<User *, ServiceError>
MessagingService::get_user_by_id(const std::string &user_id) {
  if (users.find(user_id) != users.end())
    return &users[user_id];
  return std::unexpected(ServiceError::UserNotFound);
}

std::expected<User *, ServiceError>
MessagingService::get_user_by_name(const std::string &username) {
  auto it = username_to_id.find(username);
  if (it != username_to_id.end())
    return std::unexpected(ServiceError::UserNotFound);
  if (users.find(it->second) != users.end())
    return &users[it->second];
  return std::unexpected(ServiceError::UserNotFound);
}

std::expected<std::string, ServiceError>
MessagingService::get_user_id_by_name(const std::string &username) {
  auto it = username_to_id.find(username);
  if (it != username_to_id.end()) {
    return it->second;
  }
  return std::unexpected(ServiceError::UserNotFound);
}

std::expected<std::string, ServiceError>
MessagingService::get_user_id_by_fd(int fd) {
  if (connections.find(fd) != connections.end())
    return connections[fd];
  return std::unexpected(ServiceError::UserNotFound);
}

std::string MessagingService::generate_chat_id() {
  // ВРЕМЕННО
  return "chat_" + std::to_string(chats.size() + 1);
}

std::expected<Chat *, ServiceError>
MessagingService::get_chat_by_id(const std::string &chat_id) {
  if (chats.find(chat_id) != chats.end())
    return &chats[chat_id];
  return std::unexpected(ServiceError::ChatNotFound);
}

std::expected<Chat *, ServiceError>
MessagingService::get_chat_by_name(const std::string &chat_name) {
  if (chat_name_to_id.find(chat_name) == chat_name_to_id.end())
    return std::unexpected(ServiceError::ChatNotFound);
  if (chats.find(chat_name_to_id[chat_name]) == chats.end())
    return std::unexpected(ServiceError::ChatNotFound);
  return &chats[chat_name_to_id[chat_name]];
}

std::expected<std::string, ServiceError>
MessagingService::get_chat_id_by_name(const std::string &chat_name) {
  /* for (auto &[chat_id, chat_obj] : chats) {
    if (chat_obj.get_name() == chat_name) {
      return chat_id;
    }
  }
  return "";
  if (chat_name_to_id.find(chat_name) != chat_name_to_id.end()) {
    return chat_name_to_id[chat_name];
  }
  return std::unexpected(ServiceError::ChatNotFound);
}

bool MessagingService::is_member_of_chat(const std::string &chat_name,
                                         const std::string &user_id) {
  auto chat_id = get_chat_id_by_name(chat_name);
  if (!chat_id.has_value())
    return false;
  auto chat = get_chat_by_id(*chat_id);
  if (!chat_id.has_value())
    return false;
  return (*chat)->is_member(user_id);
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
} */

ServiceResult<std::string>
UserService::register_user(const std::string &name,
                           const std::vector<uint8_t> &pubkey) {
  User user_obj("", name);
  if (users_.find(name) != users_.end())
    return std::unexpected(ServiceError::AlreadyExists);
  users_[name] = std::make_unique<User>(user_obj);
  set_public_key(name, pubkey);
  // ПОКА ЭТО НЕ НУЖНО: name_to_id_[name] = "";
  return name;
}

ServiceResult<User *> UserService::find_user(const UserIdentifier &id) {
  auto it = users_.find(id);
  if (it == users_.end())
    return std::unexpected(ServiceError::UserNotFound);
  return users_[id].get();
}

void UserService::set_public_key(const UserIdentifier &id,
                                 const std::vector<uint8_t> &pubkey) {
  auto it = users_.find(id);
  if (it == users_.end())
    return;
  it->second->set_public_key(pubkey);
}

void UserService::remove_user(const UserIdentifier &id) {
  auto it = users_.find(id);
  if (it == users_.end())
    return;
  users_.erase(it);
}

ServiceResult<std::string>
ChatService::create_chat(const std::string &name,
                         const std::string &creator_id) {
  auto it = chats_.find(name);
  if (it != chats_.end())
    return std::unexpected(ServiceError::AlreadyExists);
  Chat chat("", name, ChatType::Group);
  chats_[name] = std::make_unique<Chat>(chat);
  // Тоже пока не нужно: chat_name_to_id_[name] = generate_chat_id();
  return name;
}

ServiceResult<void> ChatService::add_member(const std::string &chat_id,
                                            const std::string &user_id) {
  auto it = chats_.find(chat_id);
  if (it == chats_.end())
    return std::unexpected(ServiceError::ChatNotFound);
  chats_[chat_id]->add_member(user_id);
  return {};
}

ServiceResult<void> ChatService::post_message(const std::string &chat_id,
                                              const std::string &username,
                                              const Message &message) {
  auto it = chats_.find(chat_id);
  if (it == chats_.end())
    return std::unexpected(ServiceError::ChatNotFound);
  if (!it->second->is_member(username))
    return std::unexpected(ServiceError::AccessDenied);
  chats_[chat_id]->addMessage(message);
  return {};
}

void ChatService::remove_chat(const std::string &chat_id) {
  auto it = chats_.find(chat_id);
  if (it == chats_.end())
    return;
  chats_.erase(it);
}

void ChatService::remove_user_from_all_chats(const std::string &username) {
  for (auto &[id, chat] : chats_) {
    if (chat->is_member(username)) {
      chat->remove_member(username);
    }
  }
}

void SessionManager::bind(int fd, const std::string &username) {
  fd_to_name_[fd] = username;
  name_to_fd_[username] = fd;
}

void SessionManager::unbind(int fd) {
  if (fd_to_name_.find(fd) == fd_to_name_.end())
    return;
  name_to_fd_.erase(fd_to_name_[fd]);
  fd_to_name_.erase(fd);
}

ServiceResult<std::string> SessionManager::get_username(int fd) const {
  if (fd_to_name_.find(fd) == fd_to_name_.end())
    return std::unexpected(ServiceError::UserNotFound);
  return fd_to_name_.find(fd)->second;
}

ServiceResult<int> SessionManager::get_fd(const std::string &username) const {
  if (name_to_fd_.find(username) == name_to_fd_.end())
    return std::unexpected(ServiceError::UserNotFound);
  return name_to_fd_.find(username)->second;
}
