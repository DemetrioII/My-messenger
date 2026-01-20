#pragma once
#include "../encryption/AESGCMEncryption.hpp"
#include "../encryption/KDF.hpp"
#include "../encryption/identity_key.hpp"
#include "../models/chat.hpp"
#include "../models/user.hpp"
#include "../network/protocol/parser.hpp"
#include "../network/transport/client.hpp"
#include "command_interface.hpp"
#include "commands_impl/exit.hpp"
#include "commands_impl/get_pubkey.hpp"
#include "commands_impl/help_command.hpp"
#include "commands_impl/join.hpp"
#include "commands_impl/login.hpp"
#include "commands_impl/make_room.hpp"
#include "commands_impl/private_message.hpp"
#include "commands_impl/send_group_message.hpp"
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
#include "message_queue.hpp"
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

class MessagingClient {
  ClientMessageDispatcher dispatcher;
  std::shared_ptr<ClientContext> context;

private:
  static uint64_t from_bytes(const std::vector<uint8_t> &v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; i++)
      r |= (uint64_t)(v[i]) << (i * 8);
    return r;
  }

  void handle_command(const std::string &msg) {
    ParsedCommand cmd_struct = context->parser.parse_line(msg);
  }

public:
  MessagingClient() : context(std::make_shared<ClientContext>()) {
    dispatcher.registerHandler(MessageType::Text,
                               std::make_unique<TextMessageHandler>());
    dispatcher.registerHandler(MessageType::Command,
                               std::make_unique<MessageCommandHandler>());
    dispatcher.registerHandler(MessageType::Response,
                               std::make_unique<ResponseMessageHandler>(
                                   context->my_username,
                                   context->encryption_service, context->mq,
                                   context->client, context->serializer));

    dispatcher.registerHandler(MessageType::CipherMessage,
                               std::make_unique<CipherMessageHandler>());
    dispatcher.registerHandler(MessageType::FileStart,
                               std::make_unique<FileStartHandler>());
    dispatcher.registerHandler(MessageType::FileChunk,
                               std::make_unique<FileChunkHandler>());
    dispatcher.registerHandler(MessageType::FileEnd,
                               std::make_unique<FileEndHandler>());
  }

  int init_client(const std::string &server_ip, int port) {
    if (!context->client->connect(server_ip, port))
      return 0;
    context->client->set_data_callback(
        [this](const std::vector<uint8_t> &data) {
          on_tcp_data_received(data);
        });
    return 1;
  }

  /* void on_send_chat_msg(const std::vector<uint8_t> &chat_name,
                        const std::vector<uint8_t> &msg) {
    ParsedCommand cmd_struct{.name = "send", .args = {chat_name, msg}};
    Message message = parser.make_command_from_struct(cmd_struct);
    client->send_to_server(serializer->serialize(message));
  }

  void send_private_msg(const std::vector<uint8_t> &recipient_identifier,
                        const std::vector<uint8_t> &msg) {
    ParsedCommand cmd_struct = {.name = "getpub",
                                .args = {recipient_identifier}};
    Message get_pubkey_msg = parser.make_command_from_struct(cmd_struct);
    client->send_to_server(serializer->serialize(get_pubkey_msg));

    std::cout << "[Crypto] Requested public key for '"
              << std::string(recipient_identifier.begin(),
                             recipient_identifier.end())
              << "'" << std::endl;

    mq->push(recipient_identifier, msg);
  } */

  void on_tcp_data_received(const std::vector<uint8_t> &raw_data) {
    Message msg = context->serializer->deserialize(raw_data);
    dispatcher.setContext(context);
    dispatcher.dispatch(msg);
    // std::string message(raw_data.begin(), raw_data.end());
    /*switch (msg.get_type()) {
    case MessageType::Text:
      dispatcher.dispatch(msg);
      break;
    case MessageType::Command: {

      break;
    }

    case MessageType::Response: {
       if (msg.get_meta(0)[0] == static_cast<uint8_t>(CommandType::GET_ID)) {
        my_username = msg.get_meta(1);
        std::string username(my_username.begin(), my_username.end());
        std::cout << "Your username is " << username << std::endl;
      } else if (msg.get_meta(0)[0] ==
                 static_cast<uint8_t>(CommandType::GET_PUBKEY)) {
        std::cout << "You got a public key" << std::endl;
        auto other_pubkey = IdentityKey::from_public_bytes(msg.get_payload());
        auto username = msg.get_meta(1);
        encryption_service->cache_public_key(username, msg.get_payload());
        auto pending_it = mq->find_pending(username);
        if (pending_it) {

          auto msg_to_send = *pending_it;
          auto ciphertext =
              encryption_service->encrypt_for(username, msg_to_send.bytes);

          Message cipher_msg{ciphertext,
                             2,
                             {msg_to_send.recipient_id, my_username},
                             MessageType::CipherMessage};

          client->send_to_server(serializer->serialize(cipher_msg));
          std::cout << "[Crypto] Sent queued message to "
                    << std::string(username.begin(), username.end())
                    << std::endl;
        }

        std::vector<std::vector<uint8_t>> pending_received;
        {
          std::lock_guard<std::mutex> lock(pending_messages_mutex);
          auto it = pending_messages.find(username);
          if (it != pending_messages.end()) {
            pending_received = std::move(it->second);
            pending_messages.erase(it);
          }
        }

        for (const auto &pending_ciphertext : pending_received) {
          encryption_service->cache_public_key(username, msg.get_payload());
          auto plaintext =
              encryption_service->decrypt_for(my_username, pending_ciphertext);
          auto plain_str = std::string(plaintext.begin(), plaintext.end());
          std::cout << plain_str << std::endl;
        }
      }
      dispatcher.setContext(context);
      dispatcher.dispatch(msg);
      break;
    }

    case MessageType::CipherMessage: {
      std::cout << "You've got a private message" << std::endl;
      dispatcher.setContext(context);
      dispatcher.dispatch(msg);
      break;
    }

    case MessageType::FileStart: {
      auto recipient = msg.get_meta(0);
      auto fname = msg.get_meta(1);
      auto size_bytes = msg.get_meta(2);
      uint64_t file_size = from_bytes(size_bytes);

      auto name = std::string(fname.begin(), fname.end());
      std::cout << "[File] Receiving " << name << " (" << file_size << " bytes)"
                << std::endl;

      std::string recipient_username_str =
          std::string(recipient.begin(), recipient.end());

      pending_files[recipient_username_str + " " + name].open(
          recipient_username_str + " " + name, std::ios::binary);
      dispatcher.setContext(context);
      dispatcher.dispatch(msg);
      break;
    }

    case MessageType::FileChunk: {
      dispatcher.setContext(context);
      dispatcher.dispatch(msg);
      break;
    }

    case MessageType::FileEnd: {
      auto fname = msg.get_meta(1);
      auto recipient = msg.get_meta(0);
      auto name = std::string(fname.begin(), fname.end());

      std::string recipient_username_str =
          std::string(recipient.begin(), recipient.end());

      // pending_files[recipient_username_str + " " + name].close();

      // pending_files.erase(recipient_username_str + " " + name);

      std::cout << "[File] Completed: " << name << std::endl;
      dispatcher.setContext(context);
      dispatcher.dispatch(msg);
      break;
    }

    case MessageType::HandShake: {
      //  НАПИСАТЬ ПО-НОВОЙ
      break;
    }
    } */
  }

  void get_data(const std::string &data) {
    Message msg = context->parser.parse(data);
    auto bytes = context->serializer->serialize(msg);
    if (msg.get_type() == MessageType::Command) {
      dispatcher.setContext(context);
      dispatcher.dispatch(msg);
    } else if (msg.get_type() == MessageType::Text) {
      context->client->send_to_server(bytes);
    }
  }

  void run() { context->client->run_event_loop(); }
};
