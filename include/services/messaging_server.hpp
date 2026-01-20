#pragma once
#include "../encryption/AESGCMEncryption.hpp"
#include "../encryption/identity_key.hpp"
#include "../models/chat.hpp"
#include "../models/user.hpp"
#include "../network/protocol/parser.hpp"
#include "../network/transport/client.hpp"
#include "../network/transport/server.hpp"
#include "command_interface.hpp"
#include "commands_impl/get_pubkey.hpp"
#include "commands_impl/join.hpp"
#include "commands_impl/login.hpp"
#include "commands_impl/make_room.hpp"
#include "commands_impl/sendfile.hpp"
#include "message_dispatcher.hpp"
#include "message_dispatchers/text_dispatcher.hpp"

#include "../utils/base64.hpp"
#include "client_context.hpp"
#include "encryption_service.hpp"
#include "message_dispatchers/cipher_handler.hpp"
#include "message_dispatchers/command_handler.hpp"
#include "message_dispatchers/file_handlers.hpp"
#include "message_dispatchers/response_handler.hpp"

#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

class MessagingServer {
private:
  std::shared_ptr<ServerContext> server_context;
  ServerMessageDispatcher dispatcher;

  static uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

public:
  MessagingServer() : server_context(std::make_shared<ServerContext>()) {
    dispatcher.registerHandler(MessageType::Text,
                               std::make_unique<TextMessageHandler>());

    dispatcher.registerHandler(MessageType::Command,
                               std::make_unique<MessageCommandHandler>());

    dispatcher.registerHandler(MessageType::CipherMessage,
                               std::make_unique<CipherMessageHandler>());

    dispatcher.registerHandler(MessageType::FileStart,
                               std::make_unique<FileStartHandler>());

    dispatcher.registerHandler(MessageType::FileChunk,
                               std::make_unique<FileChunkHandler>());

    dispatcher.registerHandler(MessageType::FileEnd,
                               std::make_unique<FileEndHandler>());
    /*command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::LOGIN);
        },
        [this](const auto &e) {
          int fd = e.first;
          const Message &msg = e.second;

          LoginCommand cmd;
          cmd.fromMessage(msg);

          Message response = cmd.execeuteOnServer(fd, messaging_service);
          transport_server->send(fd, serializer.serialize(response));
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::GET_PUBKEY);
        },
        [this](const auto &e) {
          int fd = e.first;
          const Message &msg = e.second;

          GetPubkeyCommand cmd;
          cmd.fromMessage(msg);

          Message response = cmd.execeuteOnServer(fd, messaging_service);
          transport_server->send(fd, serializer.serialize(response));
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::GET_ID);
        },
        [this](const auto &e) {
          int fd = e.first;
          if (!messaging_service.is_authenticated(fd)) {
            Message error_msg = parser.parse("User not found");
            transport_server->send(fd, serializer.serialize(error_msg));
            return;
          }

          std::string user_id = messaging_service.get_user_id_by_fd(fd);
          user_id = messaging_service.get_user_id_by_fd(fd);
          Message msg({}, 2,
                      {std::vector<uint8_t>(
                           1, static_cast<uint8_t>(CommandType::GET_ID)),
                       std::vector<uint8_t>(user_id.begin(), user_id.end())},
                      MessageType::Response);
          transport_server->send(fd, serializer.serialize(msg));
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::MAKE_ROOM);
        },
        [this](const auto &e) {
          int fd = e.first;
          const Message &msg = e.second;

          MakeRoomCommand cmd;
          cmd.fromMessage(msg);

          Message response = cmd.execeuteOnServer(fd, messaging_service);
          transport_server->send(fd, serializer.serialize(response));
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::SEND);
        },
        [this](const auto &e) {
          int fd = e.first;
          std::string user_id = messaging_service.get_user_id_by_fd(fd);
          std::string chat_id = std::string(e.second.get_meta(1).begin(),
                                            e.second.get_meta(1).end());
          chat_id = messaging_service.chat_id_by_name(chat_id);

          if (chat_id == "") {
            Message error_msg =
                parser.parse("Chat " + chat_id + "was not found");
            transport_server->send(fd, serializer.serialize(error_msg));
            return;
          }

          Message msg(e.second.get_payload(), 0, {});

          messaging_service.send_message(user_id, chat_id, msg);
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::EXIT);
        },
        [this](const auto &e) {
          int fd = e.first;
          messaging_service.remove_user_by_fd(fd);
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::JOIN);
        },
        [this](const auto &e) {
          int fd = e.first;
          const Message &msg = e.second;

          JoinCommand cmd;
          cmd.fromMessage(msg);

          Message response = cmd.execeuteOnServer(fd, messaging_service);
          transport_server->send(fd, serializer.serialize(response));
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::SEND_FILE);
        },
        [this](const auto &e) {
          int fd = e.first;
          const Message &msg = e.second;

          SendFileCommand cmd;
          cmd.fromMessage(msg);

          Message response = cmd.execeuteOnServer(fd, messaging_service);
          // transport_server->send(fd, serializer.serialize(response));
        });

    command_stream.subscribe(
        [](const auto &e) {
          return e.second.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::PRIVATE_MESSAGE);
        },
        [this](const auto &e) {
          int fd = e.first;
          std::string user_id = messaging_service.get_user_id_by_fd(fd);

          if (!messaging_service.is_authenticated(fd)) {
            transport_server->send(
                fd, serializer.serialize(parser.parse("Authenticate first")));
            return;
          }

          std::string recipient_username = std::string(
              e.second.get_meta(1).begin(), e.second.get_meta(1).end());
          std::vector<uint8_t> cipher_msg = e.second.get_payload();

          // Находим получателя
          std::string recipient_id =
              messaging_service.get_user_by_name(recipient_username);
          if (recipient_id.empty()) {
            transport_server->send(
                fd, serializer.serialize(parser.parse("Recipient not found")));
            return;
          }

          // Берём публичный ключ отправителя для расшифровки
          auto sender_user = messaging_service.get_user_by_id(user_id);
          auto sender_pubkey_opt = sender_user.get_public_key();
          if (sender_pubkey_opt.empty()) {
            transport_server->send(fd, serializer.serialize(parser.parse(
                                           "Sender key not registered")));
            return;
          }

          // Отправляем получателю
          int recipient_fd = messaging_service.get_fd_by_user_id(recipient_id);
          // transport_server->send(recipient_fd,
          // Message(sender_user.get_username() + " " + sender_pubkey_opt,
          // MessageType::HandShake));
          Message forward_msg(cipher_msg, 1, {e.second.get_meta(1)},
                              MessageType::CipherMessage);
          transport_server->send(recipient_fd,
                                 serializer.serialize(forward_msg));

          std::cout << "[PM] " << user_id << " -> " << recipient_id << ": "
                    << std::string(cipher_msg.begin(), cipher_msg.end())
                    << std::endl;
        });*/

    server_context->messaging_service->user_logged_in.subscribe(
        [](const User &) { return true; },
        [this](const User &user) {
          int fd = server_context->messaging_service->get_fd_by_user_id(
              user.get_id());
          Message welcome_msg =
              server_context->parser.parse("Hello, " + user.get_id() + "!");
          server_context->transport_server->send(
              fd, server_context->serializer.serialize(welcome_msg));
        });

    server_context->messaging_service->message_sent.subscribe(
        [](auto &) { return true; },
        [this](auto &tpl) {
          auto [from_user_id, chat_id, msg] = tpl;
          auto &chat = server_context->messaging_service->get_chat(chat_id);
          for (auto &member : chat.get_members()) {
            int fd =
                server_context->messaging_service->get_fd_by_user_id(member);
            Message m = server_context->parser.parse(
                "Message from " + from_user_id + " in chat " + chat_id + " ");
            server_context->transport_server->send(
                fd, server_context->serializer.serialize(m));
          }
        });
  }

  void start_server(int port) {
    server_context->transport_server->start(port);
    server_context->transport_server->set_data_callback(
        [this](int fd, auto data) { on_tcp_data_received(fd, data); });
  }

  void run() { server_context->transport_server->run_event_loop(); }

  void on_tcp_data_received(int fd, std::vector<uint8_t> &raw_data) {
    Message msg = server_context->serializer.deserialize(raw_data);

    /*
std::string user_id =
    server_context->messaging_service->get_user_id_by_fd(fd);
switch (msg.get_type()) {
case MessageType::Text:
  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
  // transport_server->send(fd, raw_data);
  break;
case MessageType::Command: {
  // Для всех остальных команд (login и т.д.) используем обычный поток
  // command_stream.emit({fd, msg});
  // transport_server->send(fd, raw_data);
  //
  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
  break;
}
case MessageType::HandShake:
  if (!user_id.empty()) {
    server_context->messaging_service->get_user_by_id(user_id)
        .set_public_key(msg.get_payload());
  }
  std::cout << "[Crypto] Public Key " << " registred for: " << user_id
            << std::endl;
  break;
case MessageType::CipherMessage: {
  auto recipient_username = msg.get_meta(0);
  auto recipient_id =
      std::string(recipient_username.begin(), recipient_username.end());
  recipient_id =
      server_context->messaging_service->get_user_by_name(recipient_id);
  auto recipient_fd =
      server_context->messaging_service->get_fd_by_user_id(recipient_id);
  auto &sender = server_context->messaging_service->get_user_by_id(user_id);
  Message msg_to_send(
      {msg.get_payload(),
       3,
       {msg.get_meta(0), msg.get_meta(1), sender.get_public_key()},
       MessageType::CipherMessage});
  std::cout << "Message was sent to " << recipient_id << std::endl;
  server_context->transport_server->send(
      recipient_fd, server_context->serializer.serialize(msg_to_send));
  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
  break;
}

case MessageType::FileStart: {
  auto recipient = msg.get_meta(0);
  auto fname = msg.get_meta(1);
  auto size_bytes = msg.get_meta(2);
  uint64_t file_size = from_bytes(size_bytes);

  auto name = std::string(fname.begin(), fname.end());
  std::cout << "[File] Server Receiving " << name << " (" << file_size
            << " bytes)" << std::endl;

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  int recipient_fd = server_context->messaging_service->get_fd_by_user_id(
      server_context->messaging_service->get_user_by_name(
          recipient_username_str));
  server_context->transport_server->send(
      recipient_fd, server_context->serializer.serialize(msg));
  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
  break;
}

case MessageType::FileChunk: {
  auto recipient = msg.get_meta(0);
  auto fname = msg.get_meta(1);
  auto name = std::string(fname.begin(), fname.end());

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  int recipient_fd = server_context->messaging_service->get_fd_by_user_id(
      server_context->messaging_service->get_user_by_name(
          recipient_username_str));

  std::cout << "[File] Server file chunk is sending " << name << std::endl;
  server_context->transport_server->send(
      recipient_fd, server_context->serializer.serialize(msg));
  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
  break;
}

case MessageType::FileEnd: {
  auto fname = msg.get_meta(1);
  auto recipient = msg.get_meta(0);
  auto name = std::string(fname.begin(), fname.end());

  std::string recipient_username_str =
      std::string(recipient.begin(), recipient.end());

  int recipient_fd = server_context->messaging_service->get_fd_by_user_id(
      server_context->messaging_service->get_user_by_name(
          recipient_username_str));

  std::cout << "[File] Completed: " << name << std::endl;
  server_context->transport_server->send(
      recipient_fd, server_context->serializer.serialize(msg));

  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
  break;
}
}
*/
    server_context->fd = fd;
    dispatcher.setContext(server_context);
    dispatcher.dispatch(msg);
  }

  void send_error(int fd, const std::string &error_string) {
    server_context->transport_server->send(fd, {'e', 'r', 'r', 'o', 'r'});
  }
};
