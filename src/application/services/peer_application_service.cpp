#include "include/application/services/peer_application_service.hpp"

void PeerApplicationService::disconnect_peer(
    const std::string & /*username*/,
    const std::shared_ptr<PeerContext> /*context*/) const {
  /*auto fd_res = context->session_manager->get_fd(username);
  if (!fd_res.has_value()) {
    return;
  }
  send_to_peer(*context->peer_node, *fd_res,
               context->serializer->serialize(
                   Message({}, 1,
  {{static_cast<uint8_t>(CommandType::DISCONNECT)}}, MessageType::Command)));
  disconnect_from_peer(*context->peer_node, *fd_res);
  context->user_service->remove_user(username);
  context->session_manager->unbind(*fd_res);*/
}

void PeerApplicationService::send_private_message(
    const std::vector<uint8_t> & /*recipient*/,
    const std::vector<uint8_t> & /*payload*/,
    const std::shared_ptr<PeerContext> /*context*/) const {
  /*auto res_fd =
      context->session_manager->get_fd(std::string(recipient.begin(),
  recipient.end())); if (!res_fd.has_value()) { return;
  }
  auto cipher_message = context->encryption_service->encrypt_for(
      context->my_username, recipient, payload,
  context->messages_counter[recipient]); send_to_peer(*context->peer_node,
  *res_fd, context->serializer->serialize( {cipher_message, 2,
                    {context->my_username, context->encryption_service->sign()},
                    MessageType::CipherMessage})); */
}
