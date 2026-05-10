#include "include/application/services/chat_service.hpp"

ServiceResult<std::string>
ChatService::create_chat(const std::string &name,
                         const std::string &creator_id) {
  auto it = chats_.find(name);
  if (it != chats_.end())
    return std::unexpected(ServiceError::AlreadyMember);
  Chat chat("", name, ChatType::Group);
  chats_[name] = std::make_unique<Chat>(chat);
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
