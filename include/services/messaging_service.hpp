// messaging_service.hpp
#pragma once
#include "../encryption/AESGCMEncryption.hpp"
#include "../encryption/identity_key.hpp"
#include "../models/chat.hpp"
#include "../models/user.hpp"
#include "../network/protocol/parser.hpp"
#include "../network/transport/client.hpp"
#include "../network/transport/server.hpp"
#include <expected>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

template <typename T> class Stream {
public:
  using Predicate = std::function<bool(const T &)>;
  using Handler = std::function<void(const T &)>;

  void subscribe(Predicate when, Handler then) {
    subscribers.push_back({std::move(when), std::move(then)});
  }

  void emit_subsriber_event(const T &value) {
    for (auto &s : subscribers)
      if (s.when(value))
        s.then(value);
  }

private:
  struct Subscriber {
    Predicate when;
    Handler then;
  };

  std::vector<Subscriber> subscribers;
};

enum class ServiceError {
  UserNotFound,
  ChatNotFound,
  AccessDenied,
  InvalidToken,
  AlreadyExists,
  AlreadyMember
};

template <typename T> using ServiceResult = std::expected<T, ServiceError>;

using UserIdentifier = std::string;

class UserService {
public:
  ServiceResult<std::string>
  register_user(const std::string &name, const std::vector<uint8_t> &DH_pubkey,
                const std::vector<uint8_t> &identity_pub,
                const std::vector<uint8_t> &signature);

  ServiceResult<User *> find_user(const UserIdentifier &id);

  void set_public_key(const UserIdentifier &id, const std::vector<uint8_t> &key,
                      const std::vector<uint8_t> &identity_pub,
                      const std::vector<uint8_t> &signature);

  void remove_user(const UserIdentifier &id);

private:
  std::unordered_map<std::string, std::unique_ptr<User>> users_;
  std::unordered_map<std::string, std::string> name_to_id_;
};

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

class SessionManager {
public:
  void bind(int fd, const std::string &username);

  void unbind(int fd);

  ServiceResult<std::string> get_username(int fd) const;

  ServiceResult<int> get_fd(const std::string &username) const;

private:
  std::unordered_map<int, std::string> fd_to_name_;
  std::unordered_map<std::string, int> name_to_fd_;
};

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

class PeerApplicationService {
public:
  void disconnect_peer(const std::string &username,
                       const std::shared_ptr<PeerContext> context) const;

  void send_private_message(const std::vector<uint8_t> &recipient,
                            const std::vector<uint8_t> &payload,
                            const std::shared_ptr<PeerContext> context) const;
};
