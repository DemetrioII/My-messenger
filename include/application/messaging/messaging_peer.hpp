#pragma once

#include "include//application/dispatching/message_dispatchers/command_handler.hpp"
#include "include/application/dispatching/message_dispatcher.hpp"
#include "include/application/dispatching/message_dispatchers/peer_cipher_handler.hpp"
#include "include/application/dispatching/message_dispatchers/peer_file_handlers.hpp"
#include "include/application/dispatching/message_dispatchers/peer_response_handler.hpp"
#include "include/application/dispatching/message_dispatchers/peer_text_handler.hpp"
#include "include/application/session/client_context.hpp"
#include "include/protocol/parser.hpp"
#include "include/transport/peer.hpp"

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
