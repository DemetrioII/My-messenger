#include "../../../include/services/commands_impl/get_pubkey.hpp"

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
  auto transport_server = context->transport_server;
  int fd = context->fd;
  if (username.empty()) {
    transport_server->send(fd, StaticResponses::WRONG_COMMAND_USAGE);
    return;
  }

  auto user_service = context->user_service;
  auto session_manager = context->session_manager;
  auto user_res =
      user_service->find_user(std::string(username.begin(), username.end()));
  if (!user_res.has_value()) {
    transport_server->send(fd, StaticResponses::USER_NOT_FOUND);
    return;
  }
  Message msg(
      {(*user_res)->get_public_key(),
       2,
       {std::vector<uint8_t>{static_cast<uint8_t>(CommandType::GET_PUBKEY)},
        username},
       MessageType::Response});
  context->transport_server->send(context->fd,
                                  context->serializer.serialize(msg));
}

void GetPubkeyCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  std::shared_ptr<IClient> client = context->client;
  std::shared_ptr<Serializer> serializer = context->serializer;
  client->send_to_server(serializer->serialize(toMessage()));
}

GetPubkeyCommand::~GetPubkeyCommand() {}
