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

  std::string get_user_id_by_token(std::string &token) {
    auto it = auth_tokens.find(token);
    if (it != auth_tokens.end())
      return it->second;
    return "";
  }

  void remove_user_by_fd(int fd) {
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

  void bind_connection(int fd, const std::string &user_id) {
    connections[fd] = user_id;
    fds[user_id] = fd;
  }

  bool is_authenticated(int fd) {
    return connections.find(fd) != connections.end();
  }

  int get_fd_by_user_id(const std::string &user_id) { return fds[user_id]; }

  std::string chat_id_by_name(std::string &chat_name) {
    for (auto &chat : chats) {
      if (chat.second.get_name() == chat_name)
        return chat.second.get_id();
    }
    return "";
  }

  bool send_message(const std::string &from_user_id,
                    const std::string &to_chat_id, const Message &message) {
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

  bool join_chat_by_name(const std::string &user_id,
                         const std::string &chat_name) {
    for (auto &[id, chat] : chats) {
      if (chat.get_name() == chat_name) {
        chat.add_member(user_id);
        users[user_id].join_chat(id);

        return true;
      }
    }
    return false;
  }

  User &get_user_by_id(std::string user_id) { return users[user_id]; }

  std::string get_user_by_name(std::string username) {
    for (auto &u : users) {
      if (u.second.get_username() == username)
        return u.first;
    }
    return "";
  }

  std::string get_user_id_by_fd(int fd) {
    if (connections.find(fd) != connections.end())
      return connections[fd];
    else
      return "";
  }

  std::string generate_chat_id() {
    // ВРЕМЕННО
    return "chat_" + std::to_string(chats.size() + 1);
  }

  auto &get_chat_by_id(const std::string &chat_id) { return chats[chat_id]; }

  std::string get_chat_id_by_name(const std::string &chat_name) {
    for (auto &[chat_id, chat_obj] : chats) {
      if (chat_obj.get_name() == chat_name) {
        return chat_id;
      }
    }
    return "";
  }

  bool is_member_of_chat(const std::string &chat_name,
                         const std::string &user_id) {
    Chat &chat = get_chat_by_id(get_chat_id_by_name(chat_name));
    return chat.is_member(user_id);
  }

  std::string create_chat(const std::string &creator_id,
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

private:
  std::string generate_token() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::stringstream ss;
    ss << std::hex << dis(gen) << dis(gen);
    return ss.str();
  }
};
