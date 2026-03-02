#pragma once

#include "../network/protocol/parser.hpp"
#include "client_context.hpp"

class MessagingPeer {
  std::shared_ptr<PeerContext> context_;

public:
  MessagingPeer() : context_() {}
};
