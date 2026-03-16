#include "../../../include/services/commands_impl/disconnect.hpp"

CommandType DisconnectCommand::getType() const {
  return CommandType::DISCONNECT;
}

void DisconnectCommand::fromParsedCommand(const ParsedCommand &pc) {}

Message DisconnectCommand::toMessage() const {
  return Message(
      {{}, 1, {{static_cast<uint8_t>(getType())}}, MessageType::Command});
}

void DisconnectCommand::fromMessage(const Message &msg) {
  username = std::string(msg.get_payload().begin(), msg.get_payload().end());
}

void DisconnectCommand::execeuteOnServer(
    std::shared_ptr<ServerContext> context) {}

void DisconnectCommand::executeOnClient(
    std::shared_ptr<ClientContext> context) {}

void DisconnectCommand::send_from_peer(int fd,
                                       std::shared_ptr<PeerContext> context) {
  if (username.empty()) {
    std::cout << "Usage: disconnect <username>" << std::endl;
    return;
  }
  auto fd_res = context->session_manager->get_fd(username);
  if (!fd_res.has_value()) {
    switch (fd_res.error()) {
    case ServiceError::UserNotFound: {
      std::cout << "User not found or not been connected" << std::endl;
      return;
    }
    default: {
      std::cout << "Unknown error" << std::endl;
      return;
    }
    }
  }
  // send_to_peer(*context->peer_node, fd, );
  fd = *fd_res;
  send_to_peer(*context->peer_node, fd,
               context->serializer->serialize(toMessage()));
  disconnect_from_peer(*context->peer_node, fd);
  context->user_service->remove_user(username);
  context->session_manager->unbind(fd);
}

void DisconnectCommand::recv_on_peer(int fd,
                                     std::shared_ptr<PeerContext> context) {
  std::cout << "Peer " << fd << " disconnected" << std::endl;
  auto username_res = *context->session_manager->get_username(fd);
  context->user_service->remove_user(username_res);
  context->session_manager->unbind(fd);
}

DisconnectCommand::~DisconnectCommand() {}
