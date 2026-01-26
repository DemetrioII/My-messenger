#pragma once

#include "../network/protocol/parser.hpp"

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

  void handle_command(const std::string &msg);

public:
  MessagingClient();

  int init_client(const std::string &server_ip, int port);

  void on_tcp_data_received(const std::vector<uint8_t> &raw_data);

  std::shared_ptr<ClientContext> get_context();

  std::vector<std::string> search_online_users(const std::string &pattern);

  std::optional<std::vector<std::vector<uint8_t>>>
  query_user_info(std::string &username);

  void get_data(const std::string &data);

  void run();
};
