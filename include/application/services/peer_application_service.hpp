#pragma once

#include "include/application/session/client_context.hpp"

class PeerApplicationService {
public:
  void disconnect_peer([[maybe_unused]] const std::string &username,
                       [[maybe_unused]] const std::shared_ptr<PeerContext> context) const;

  void send_private_message([[maybe_unused]] const std::vector<uint8_t> &recipient,
                            [[maybe_unused]] const std::vector<uint8_t> &payload,
                            [[maybe_unused]] const std::shared_ptr<PeerContext> context) const;
};
