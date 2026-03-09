#pragma once

#include "../network/protocol/parser.hpp"
#include "../network/transport/peer.hpp"
#include "client_context.hpp"
#include "message_dispatcher.hpp"
#include "message_dispatchers/cipher_handler.hpp"
#include "message_dispatchers/command_handler.hpp"
#include "message_dispatchers/file_handlers.hpp"
#include "message_dispatchers/response_handler.hpp"
#include "message_dispatchers/text_dispatcher.hpp"

class MessagingPeer {
  std::shared_ptr<PeerContext> context_;

  PeerMessageDispatcher dispatcher;

public:
  MessagingPeer(int port);

  void run();

  void stop_peer();

  void send_msg(const std::string &msg_to_send);

  ~MessagingPeer();
};
