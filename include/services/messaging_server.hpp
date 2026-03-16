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
  MessagingServer();

  void start_server(int port);

  void run();

  void on_tcp_data_received(int fd, std::vector<uint8_t> &raw_data);

  void send_error(int fd, const std::string &error_string);
};
