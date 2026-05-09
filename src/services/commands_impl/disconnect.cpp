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

void DisconnectCommand::send_from_peer(std::shared_ptr<PeerContext> context) {
  if (username.empty()) {
    std::cout << "Usage: disconnect <username>" << std::endl;
    return;
  }
  PeerApplicationService service;
  service.disconnect_peer(username, context);
}

void DisconnectCommand::recv_on_peer(int fd,
                                     std::shared_ptr<PeerContext> context) {
  std::cout << "Peer " << fd << " disconnected" << std::endl;
  auto username_res = *context->session_manager->get_username(fd);
  context->user_service->remove_user(username_res);
  context->session_manager->unbind(fd);
}

DisconnectCommand::~DisconnectCommand() {}
