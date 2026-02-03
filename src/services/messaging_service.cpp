#include "../../include/services/messageing_service.hpp"

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
  chats_[name]->add_member(creator_id);
  return name;
}

ServiceResult<void> ChatService::add_member(const std::string &chat_id,
                                            const std::string &user_id) {
  auto it = chats_.find(chat_id);
  if (it == chats_.end())
    return std::unexpected(ServiceError::ChatNotFound);
  if (chats_[chat_id]->is_member(user_id))
    return std::unexpected(ServiceError::AlreadyMember);
  chats_[chat_id]->add_member(user_id);
  return {};
}

ServiceResult<std::vector<std::string>>
ChatService::post_message(const std::string &chat_id,
                          const std::string &username, const Message &message) {
  auto it = chats_.find(chat_id);
  if (it == chats_.end())
    return std::unexpected(ServiceError::ChatNotFound);
  if (!it->second->is_member(username))
    return std::unexpected(ServiceError::AccessDenied);
  chats_[chat_id]->addMessage(message);
  return chats_[chat_id]->get_members();
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
