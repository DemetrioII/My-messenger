#include "include/application/commands/impl/get_pubkey.hpp"

CommandType GetPubkeyCommand::getType() const {
  return CommandType::GET_PUBKEY;
}

void GetPubkeyCommand::fromParsedCommand(const ParsedCommand &parsed) {
  if (!parsed.args.empty()) {
    username = parsed.args[0];
  }
}

Message GetPubkeyCommand::toMessage() const {
  return Message({}, 2, {{static_cast<uint8_t>(getType())}, username},
                 MessageType::Command);
}

void GetPubkeyCommand::fromMessage(const Message &msg) {
  username = msg.get_meta(1);
}

void GetPubkeyCommand::execeuteOnServer(
    std::shared_ptr<ServerContext> context) {
  context->app_service->send_pubkey(username);
}

void GetPubkeyCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  std::shared_ptr<IClient> client = context->client;
  std::shared_ptr<Serializer> serializer = context->serializer;
  client->send_to_server(serializer->serialize(toMessage()));
}

void GetPubkeyCommand::recv_on_peer(int fd,
                                    std::shared_ptr<PeerContext> context) {
  std::cout << "User with fd=" << fd << " requested your public keys"
            << std::endl;
  send_to_peer(
      *context->peer_node, fd,
      context->serializer->serialize(
          {context->encryption_service->get_DH_bytes(),
           4,
           {std::vector<uint8_t>{static_cast<uint8_t>(CommandType::GET_PUBKEY)},
            username, context->encryption_service->get_identity_bytes(),
            context->encryption_service->sign()},
           MessageType::Response}));
}

void GetPubkeyCommand::send_from_peer(std::shared_ptr<PeerContext> context) {
  auto res_fd = context->session_manager->get_fd(
      std::string(username.begin(), username.end()));
  switch (res_fd.error()) {
  case ServiceError::UserNotFound: {
    std::cout << "Peer not found" << std::endl;
    return;
  }
  default: {
  }
  }
  int fd = *res_fd;
  send_to_peer(*context->peer_node, fd,
               context->serializer->serialize(toMessage()));
}

GetPubkeyCommand::~GetPubkeyCommand() {}
