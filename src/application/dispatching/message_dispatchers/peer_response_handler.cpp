#include "include/application/dispatching/message_dispatchers/peer_response_handler.hpp"

void PeerResponseHandler::handle_connect_response(
    const Message &msg, const std::shared_ptr<PeerContext> context) {
  std::string username_str(msg.get_payload().begin(), msg.get_payload().end());
  const auto &dh_bytes = msg.get_meta(1);
  const auto &identity_bytes = msg.get_meta(2);
  const auto &signature = msg.get_meta(3);

  auto register_res = context->user_service->register_user(
      username_str, dh_bytes, identity_bytes, signature);
  if (!register_res.has_value()) {
    switch (register_res.error()) {
    case ServiceError::AlreadyExists:
      std::cout << "This user already exists" << std::endl;
      return;
    default:
      std::cout << "Unknown error" << std::endl;
      return;
    }
  }

  context->session_manager->bind(context->fd, username_str);
  context->encryption_service->cache_public_key(msg.get_payload(), dh_bytes,
                                                identity_bytes);
  std::cout << "You got public keys of " << username_str << std::endl;
}

void PeerResponseHandler::handleSending(const Message &msg,
                                        std::shared_ptr<PeerContext> /*context*/) {
  (void)msg;
}

void PeerResponseHandler::handleReceiving(
    const Message &msg, std::shared_ptr<PeerContext> context) {
  auto meta0 = msg.get_meta(0);
  if (meta0.empty())
    return;

  auto cmd_type = static_cast<CommandType>(meta0[0]);
  switch (cmd_type) {
  case CommandType::GET_PUBKEY: {
    break;
  }
  case CommandType::CONNECT: {
    handle_connect_response(msg, context);
    break;
  }
  default:
    break;
  }
}

MessageType PeerResponseHandler::getMessageType() const {
  return MessageType::Response;
}
