#include "../../../include/services/commands_impl/login.hpp"

CommandType LoginCommand::getType() const { return CommandType::LOGIN; }

void LoginCommand::fromParsedCommand(const ParsedCommand &parsed) {
  if (!parsed.args.empty()) {
    username = parsed.args[0];
    // pubkey_bytes = parsed.args[1];
  }
}

Message LoginCommand::toMessage() const {
  ParsedCommand pc;
  pc.name = "login";
  pc.args = {std::vector<uint8_t>(username.begin(), username.end())};
  pc.args.push_back(pubkey_bytes);

  Parser parser;
  return parser.make_command_from_struct(pc);
}

void LoginCommand::fromMessage(const Message &msg) {
  username = msg.get_meta(1);
  pubkey_bytes = msg.get_meta(2);
}

void LoginCommand::execeuteOnServer(std::shared_ptr<ServerContext> context) {
  auto service = context->messaging_service;
  auto fd = context->fd;
  auto username_string = std::string(username.begin(), username.end());
  if (service->is_authenticated(fd)) {
    std::string error_msg = "[Error]: You are already logged in";
    context->transport_server->send(
        fd, context->serializer.serialize(Message(
                std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0, {},
                MessageType::Text)));
    return;
  }
  auto token = service->authenticate(username_string, "", pubkey_bytes);
  if (token == "") {
    std::string error_msg =
        "[Error]: User " + username_string + " is already exists";
    context->transport_server->send(
        fd, context->serializer.serialize(Message(
                std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0, {},
                MessageType::Text)));
    return;
  }

  auto user_id = service->get_user_id_by_token(token);
  service->bind_connection(fd, user_id);
  auto &user = service->get_user_by_id(user_id);
  user.set_public_key(pubkey_bytes);
  if (user.get_public_key().empty()) {
    std::string error_msg = "[Error]: Public key has not been set";
    context->transport_server->send(
        fd, context->serializer.serialize(Message(
                std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0, {},
                MessageType::Text)));
    return;
  }
  std::string response_string = "Hello, " + username_string + "!";
  context->transport_server->send(
      fd,
      context->serializer.serialize(Message(
          std::vector<uint8_t>(response_string.begin(), response_string.end()),
          0, {}, MessageType::Text)));
}

void LoginCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  std::shared_ptr<IClient> client = context->client;
  std::shared_ptr<EncryptionService> encryptionService =
      context->encryption_service;
  pubkey_bytes = encryptionService->get_public_bytes();
  context->my_username = username;

  Serializer serializer;
  client->send_to_server(serializer.serialize(toMessage()));
}

LoginCommand::~LoginCommand() {}
