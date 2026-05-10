#include "include/application/services/server_application_service.hpp"

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
