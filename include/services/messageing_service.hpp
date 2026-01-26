// messaging_service.hpp
#pragma once
#include "../encryption/AESGCMEncryption.hpp"
#include "../encryption/identity_key.hpp"
#include "../models/chat.hpp"
#include "../models/user.hpp"
#include "../network/protocol/parser.hpp"
#include "../network/transport/client.hpp"
#include "../network/transport/server.hpp"
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

class MessagingService {
private:
  std::unordered_map<std::string, User> users;
  std::unordered_map<std::string, Chat> chats;
  std::unordered_map<std::string, std::string> auth_tokens;
  std::unordered_map<int, std::string> connections;
  std::unordered_map<std::string, int> fds;

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

  std::string chat_id_by_name(std::string &chat_name);

  bool send_message(const std::string &from_user_id,
                    const std::string &to_chat_id, const Message &message);

  bool join_chat_by_name(const std::string &user_id,
                         const std::string &chat_name);

  User &get_user_by_id(std::string user_id);

  std::string get_user_by_name(std::string username);

  std::string get_user_id_by_fd(int fd);

  std::string generate_chat_id();

  Chat &get_chat_by_id(const std::string &chat_id);

  std::string get_chat_id_by_name(const std::string &chat_name);

  bool is_member_of_chat(const std::string &chat_name,
                         const std::string &user_id);

  std::string create_chat(const std::string &creator_id,
                          const std::string &name, ChatType type,
                          const std::vector<std::string> &initial_members);

private:
  std::string generate_token();
};
