#include "include/application/messaging/messaging_service.hpp"

ServiceResult<std::string>
UserService::register_user(const std::string &name,
                           const std::vector<uint8_t> &pubkey,
                           const std::vector<uint8_t> &identity_pub,
                           const std::vector<uint8_t> &signature) {
  User user_obj("", name);
  if (users_.find(name) != users_.end())
    return std::unexpected(ServiceError::AlreadyExists);
  users_[name] = std::make_unique<User>(user_obj);
  set_public_key(name, pubkey, identity_pub, signature);
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
                                 const std::vector<uint8_t> &pubkey,
                                 const std::vector<uint8_t> &identity_pub,
                                 const std::vector<uint8_t> &signature) {
  auto it = users_.find(id);
  if (it == users_.end())
    return;
  it->second->set_public_DH_key(pubkey);
  it->second->set_public_Identity_key(identity_pub);
  it->second->set_key_signature(signature);
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

ServerApplicationService::ServerApplicationService(
    std::shared_ptr<UserService> user_service,
    std::shared_ptr<ChatService> chat_service,
    std::shared_ptr<SessionManager> session_manager,
    std::shared_ptr<IServer> transport_server, Serializer *serializer,
    int *fd_ref)
    : user_service_(std::move(user_service)),
      chat_service_(std::move(chat_service)),
      session_manager_(std::move(session_manager)),
      transport_server_(std::move(transport_server)), serializer_(serializer),
      fd_ref_(fd_ref) {}

std::optional<std::string> ServerApplicationService::current_username() const {
  if (!fd_ref_)
    return std::nullopt;
  auto res = session_manager_->get_username(*fd_ref_);
  if (!res.has_value())
    return std::nullopt;
  return *res;
}

void ServerApplicationService::login(
    const std::string &username, const std::vector<uint8_t> &dh_pubkey,
    const std::vector<uint8_t> &identity_pub,
    const std::vector<uint8_t> &signature) const {
  if (!fd_ref_ || !serializer_ || !transport_server_)
    return;

  auto fd = *fd_ref_;
  auto username_fd_res = session_manager_->get_username(fd);
  if (username_fd_res.has_value()) {
    transport_server_->send(fd, StaticResponses::YOU_ARE_LOGGED_IN);
    return;
  }

  auto username_res = user_service_->register_user(username, dh_pubkey,
                                                   identity_pub, signature);
  if (!username_res.has_value()) {
    transport_server_->send(fd, StaticResponses::PUBLIC_KEY_HAS_NOT_SET);
    return;
  }

  session_manager_->bind(fd, username);
  std::string response_string = "Hello, " + username + "!";
  transport_server_->send(fd, serializer_->serialize(Message(
                                  std::vector<uint8_t>(response_string.begin(),
                                                       response_string.end()),
                                  0, {}, MessageType::Text)));
}

void ServerApplicationService::join_room(const std::string &chat_name) const {
  if (!fd_ref_ || !serializer_ || !transport_server_)
    return;

  auto fd = *fd_ref_;
  auto username = session_manager_->get_username(fd);
  if (!username.has_value()) {
    transport_server_->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }

  if (chat_name.empty()) {
    transport_server_->send(fd, StaticResponses::EMPTY_CHAT_NAME);
    return;
  }

  auto chat_res = chat_service_->add_member(chat_name, *username);
  if (!chat_res.has_value()) {
    if (chat_res.error() == ServiceError::ChatNotFound) {
      transport_server_->send(fd, StaticResponses::CHAT_NOT_FOUND);
      return;
    }
    if (chat_res.error() == ServiceError::AlreadyMember) {
      transport_server_->send(fd, StaticResponses::YOU_ARE_ALREADY_MEMBER);
      return;
    }
  }

  std::string response = "You joined the room " + chat_name;
  transport_server_->send(
      fd, serializer_->serialize(
              Message(std::vector<uint8_t>(response.begin(), response.end()), 0,
                      {}, MessageType::Text)));
}

void ServerApplicationService::deliver_cipher_message(int fd,
                                                      const Message &msg) {
  auto username = session_manager_->get_username(fd);
  if (!username.has_value()) {
    transport_server_->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }
  auto recipient_username =
      std::string(msg.get_meta(0).begin(), msg.get_meta(0).end());
  auto recipient_fd = session_manager_->get_fd(recipient_username);
  if (!recipient_fd.has_value()) {
    transport_server_->send(fd, StaticResponses::USER_NOT_FOUND);
    return;
  }
  auto sender = user_service_->find_user(*username);
  if (!sender.has_value()) {
    transport_server_->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }
  Message msg_to_send(
      {std::move(msg.get_payload()),
       5,
       {msg.get_meta(0), msg.get_meta(1), (*sender)->get_public_DH_key(),
        (*sender)->get_public_Identity_key(), (*sender)->get_key_signature()},
       MessageType::CipherMessage});
  std::cout << "Message was sent to " << recipient_username << std::endl;
  transport_server_->send(*recipient_fd, serializer_->serialize(msg_to_send));
}

void ServerApplicationService::create_room(const std::string &chat_name) const {
  if (!fd_ref_ || !serializer_ || !transport_server_)
    return;

  auto fd = *fd_ref_;
  auto username = session_manager_->get_username(fd);
  if (!username.has_value()) {
    transport_server_->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }

  if (chat_name.empty()) {
    transport_server_->send(fd, StaticResponses::EMPTY_CHAT_NAME);
    return;
  }

  auto chat_res = chat_service_->create_chat(chat_name, *username);
  if (!chat_res.has_value()) {
    transport_server_->send(fd, StaticResponses::CHAT_ALREADY_EXISTS);
    return;
  }

  std::string response_str = "Room " + chat_name + " was created!";
  transport_server_->send(
      fd, serializer_->serialize(Message(
              std::vector<uint8_t>(response_str.begin(), response_str.end()), 0,
              {}, MessageType::Text)));
}

void ServerApplicationService::send_group_message(
    const std::string &chat_name, const Message &message) const {
  if (!fd_ref_ || !serializer_ || !transport_server_)
    return;

  auto fd = *fd_ref_;
  auto username_res = session_manager_->get_username(fd);
  if (!username_res.has_value()) {
    transport_server_->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }

  auto send_res =
      chat_service_->post_message(chat_name, *username_res, message);
  if (!send_res.has_value()) {
    if (send_res.error() == ServiceError::AccessDenied) {
      transport_server_->send(fd, StaticResponses::YOU_ARE_NOT_MEMBER);
      return;
    }
    if (send_res.error() == ServiceError::ChatNotFound) {
      transport_server_->send(fd, StaticResponses::CHAT_NOT_FOUND);
      return;
    }
  }

  for (const auto &username : *send_res) {
    auto his_fd = session_manager_->get_fd(username);
    if (his_fd.has_value()) {
      transport_server_->send(*his_fd, serializer_->serialize(message));
    }
  }
}

void ServerApplicationService::disconnect_username(
    const std::string &username) const {
  if (!fd_ref_)
    return;
  auto fd_res = session_manager_->get_fd(username);
  if (!fd_res.has_value())
    return;
  user_service_->remove_user(username);
  session_manager_->unbind(*fd_res);
}

void ServerApplicationService::exit_current_session() const {
  if (!fd_ref_)
    return;
  auto username = session_manager_->get_username(*fd_ref_);
  if (!username.has_value())
    return;
  user_service_->remove_user(*username);
  chat_service_->remove_user_from_all_chats(*username);
  session_manager_->unbind(*fd_ref_);
}

void ServerApplicationService::send_pubkey(
    const std::vector<uint8_t> &username) const {
  if (!fd_ref_ || !serializer_ || !transport_server_)
    return;

  if (username.empty()) {
    transport_server_->send(*fd_ref_, StaticResponses::WRONG_COMMAND_USAGE);
    return;
  }

  auto username_str = std::string(username.begin(), username.end());
  auto user_res = user_service_->find_user(username_str);
  if (!user_res.has_value()) {
    transport_server_->send(*fd_ref_, StaticResponses::USER_NOT_FOUND);
    return;
  }

  Message msg(
      {(*user_res)->get_public_DH_key(),
       4,
       {std::vector<uint8_t>{static_cast<uint8_t>(CommandType::GET_PUBKEY)},
        username, (*user_res)->get_public_Identity_key(),
        (*user_res)->get_key_signature()},
       MessageType::Response});
  transport_server_->send(*fd_ref_, serializer_->serialize(msg));
}

void PeerApplicationService::disconnect_peer(
    const std::string &username,
    const std::shared_ptr<PeerContext> context) const {
  /*auto fd_res = context->session_manager->get_fd(username);
  if (!fd_res.has_value()) {
    return;
  }
  send_to_peer(*context->peer_node, *fd_res,
               context->serializer->serialize(
                   Message({}, 1,
  {{static_cast<uint8_t>(CommandType::DISCONNECT)}}, MessageType::Command)));
  disconnect_from_peer(*context->peer_node, *fd_res);
  context->user_service->remove_user(username);
  context->session_manager->unbind(*fd_res);*/
}

void PeerApplicationService::send_private_message(
    const std::vector<uint8_t> &recipient, const std::vector<uint8_t> &payload,
    const std::shared_ptr<PeerContext> context) const {
  /*auto res_fd =
      context->session_manager->get_fd(std::string(recipient.begin(),
  recipient.end())); if (!res_fd.has_value()) { return;
  }
  auto cipher_message = context->encryption_service->encrypt_for(
      context->my_username, recipient, payload,
  context->messages_counter[recipient]); send_to_peer(*context->peer_node,
  *res_fd, context->serializer->serialize( {cipher_message, 2,
                    {context->my_username, context->encryption_service->sign()},
                    MessageType::CipherMessage})); */
}
