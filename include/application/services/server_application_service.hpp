#pragma once

#include "include/application/services/chat_service.hpp"
#include "include/application/services/session_manager.hpp"
#include "include/application/services/user_service.hpp"

#include "include/protocol/parser.hpp"
#include "include/transport/server.hpp"

class ServerApplicationService {
public:
  ServerApplicationService(std::shared_ptr<UserService> user_service,
                           std::shared_ptr<ChatService> chat_service,
                           std::shared_ptr<SessionManager> session_manager,
                           std::shared_ptr<IServer> transport_server,
                           Serializer *serializer, int *fd_ref);

  void login(const std::string &username, const std::vector<uint8_t> &dh_pubkey,
             const std::vector<uint8_t> &identity_pub,
             const std::vector<uint8_t> &signature) const;

  void join_room(const std::string &chat_name) const;
  void create_room(const std::string &chat_name) const;
  void send_group_message(const std::string &chat_name,
                          const Message &message) const;
  void disconnect_username(const std::string &username) const;
  void exit_current_session() const;
  void deliver_cipher_message(int fd, const Message &msg);
  void handle_cipher_message(const Message &msg);
  void send_pubkey(const std::vector<uint8_t> &username) const;
  std::optional<std::string> current_username() const;

private:
  std::shared_ptr<UserService> user_service_;
  std::shared_ptr<ChatService> chat_service_;
  std::shared_ptr<SessionManager> session_manager_;
  std::shared_ptr<IServer> transport_server_;
  Serializer *serializer_ = nullptr;
  int *fd_ref_ = nullptr;
};
