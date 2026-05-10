#pragma once
#include "include/app/config.hpp"
#include "include/application/commands/command_interface.hpp"
#include "include/application/commands/impl/get_pubkey.hpp"
#include "include/application/commands/impl/join.hpp"
#include "include/application/commands/impl/login.hpp"
#include "include/application/commands/impl/make_room.hpp"
#include "include/application/commands/impl/sendfile.hpp"
#include "include/application/dispatching/message_dispatcher.hpp"
#include "include/application/dispatching/message_dispatchers/text_dispatcher.hpp"
#include "include/infrastructure/crypto/AESGCMEncryption.hpp"
#include "include/infrastructure/crypto/identity_key.hpp"
#include "include/models/chat.hpp"
#include "include/models/user.hpp"
#include "include/protocol/parser.hpp"
#include "include/transport/client.hpp"
#include "include/transport/server.hpp"

#include "include/application/dispatching/message_dispatchers/cipher_handler.hpp"
#include "include/application/dispatching/message_dispatchers/command_handler.hpp"
#include "include/application/dispatching/message_dispatchers/response_handler.hpp"
#include "include/application/dispatching/message_dispatchers/server_cipher_handler.hpp"
#include "include/application/dispatching/message_dispatchers/server_file_handlers.hpp"
#include "include/application/dispatching/message_dispatchers/server_text_handler.hpp"
#include "include/application/services/encryption_service.hpp"
#include "include/application/session/client_context.hpp"
#include "utils/base64.hpp"

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
  explicit MessagingServer(const AppConfig &config);

  void start_server();

  void run();

  void stop();

  void on_tcp_data_received(int fd, std::vector<uint8_t> &raw_data);

  void send_error(int fd, const std::string &error_string);
};
