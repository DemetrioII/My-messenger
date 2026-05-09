#pragma once

#include "include/application/session/client_context.hpp"

class PeerApplicationService {
public:
  void disconnect_peer(const std::string &username,
                       const std::shared_ptr<PeerContext> context) const;

  void send_private_message(const std::vector<uint8_t> &recipient,
                            const std::vector<uint8_t> &payload,
                            const std::shared_ptr<PeerContext> context) const;
};
