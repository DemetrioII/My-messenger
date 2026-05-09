#pragma once

#include "include/application/services/services_common.hpp"
#include "include/models/chat.hpp"
#include "include/models/message.hpp"

class ChatService {
public:
  [[nodiscard]]
  ServiceResult<std::string> create_chat(const std::string &name,
                                         const std::string &creator_id);

  [[nodiscard]]
  ServiceResult<void> add_member(const std::string &chat_id,
                                 const std::string &user_id);

  [[nodiscard]]
  ServiceResult<std::vector<std::string>>
  post_message(const std::string &chat_id, const std::string &username,
               const Message &msg);

  void remove_chat(const std::string &chat_id);

  void remove_user_from_all_chats(const std::string &username);

private:
  std::unordered_map<std::string, std::unique_ptr<Chat>> chats_;
  std::unordered_map<std::string, std::string> chat_name_to_id_;
};
