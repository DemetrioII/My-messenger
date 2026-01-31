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
  auto service = context->messaging_service;
  auto transport_server = context->transport_server;
  int fd = context->fd;
  if (!service->is_authenticated(fd)) {
    transport_server->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }
  if (username.empty()) {
    transport_server->send(fd, StaticResponses::WRONG_COMMAND_USAGE);
    return;
  }
  auto user_id_str =
      service->get_user_by_name(std::string(username.begin(), username.end()));
  if (user_id_str == "") {
    transport_server->send(fd, StaticResponses::USER_NOT_FOUND);
    return;
  }
  auto &user = service->get_user_by_id(user_id_str);
  Message msg(
      {user.get_public_key(),
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
