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
  ServiceResult<std::string> register_user(const std::string &name,
                                           const std::vector<uint8_t> &pubkey);

  ServiceResult<User *> find_user(const UserIdentifier &id);

  void set_public_key(const UserIdentifier &id,
                      const std::vector<uint8_t> &key);

  void remove_user(const UserIdentifier &id);

private:
  std::unordered_map<std::string, std::unique_ptr<User>> users_;
  std::unordered_map<std::string, std::string> name_to_id_;
};

class ChatService {
public:
  ServiceResult<std::string> create_chat(const std::string &name,
                                         const std::string &creator_id);

  ServiceResult<void> add_member(const std::string &chat_id,
                                 const std::string &user_id);
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

/*class MessagingService {
private:
  std::unordered_map<std::string, User> users;
  std::unordered_map<std::string, Chat> chats;
  std::unordered_map<std::string, std::string> auth_tokens;
  std::unordered_map<int, std::string> connections;
  std::unordered_map<std::string, int> fds;
  std::unordered_map<std::string, std::string> username_to_id;
  std::unordered_map<std::string, std::string> chat_name_to_id;

public:
  Stream<User> user_logged_in;
  Stream<std::pair<std::string, Chat>> chat_created;
  Stream<std::tuple<std::string, std::string, Message>> message_sent;
  MessagingService() = default;

  std::string authenticate(const std::string &username,
                           const std::string &password_hash,
                           const std::vector<uint8_t> &pubkey_bytes);

  std::string get_user_id_by_token(std::string &token);

  void remove_user_by_fd(int fd);

  void bind_connection(int fd, const std::string &user_id);

  bool is_authenticated(int fd);

  int get_fd_by_user_id(const std::string &user_id);

  bool send_message(const std::string &from_user_id,
                    const std::string &to_chat_id, const Message &message);

  std::expected<bool, ServiceError>
  join_chat_by_name(const std::string &user_id, const std::string &chat_name);

  std::expected<User *, ServiceError>
  get_user_by_id(const std::string &user_id);

  std::expected<User *, ServiceError>
  get_user_by_name(const std::string &username);

  std::expected<std::string, ServiceError>
  get_user_id_by_name(const std::string &username);

  std::expected<std::string, ServiceError> get_user_id_by_fd(int fd);

  std::string generate_chat_id();

  std::expected<Chat *, ServiceError>
  get_chat_by_id(const std::string &chat_id);

  std::expected<Chat *, ServiceError>
  get_chat_by_name(const std::string &chat_name);

  std::expected<std::string, ServiceError>
  get_chat_id_by_name(const std::string &chat_name);

  bool is_member_of_chat(const std::string &chat_name,
                         const std::string &user_id);

  std::string create_chat(const std::string &creator_id,
                          const std::string &name, ChatType type,
                          const std::vector<std::string> &initial_members);

private:
  std::string generate_token();
}; */
