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
  auto user_service = context->user_service;
  auto session_manager = context->session_manager;
  auto fd = context->fd;
  auto username_string = std::string(username.begin(), username.end());

  auto username_fd_res = session_manager->get_username(fd);
  if (username_fd_res.has_value()) {
    context->transport_server->send(fd, StaticResponses::YOU_ARE_LOGGED_IN);
    return;
  }

  auto username_res =
      user_service->register_user(username_string, pubkey_bytes);
  if (!username_res.has_value()) {
    context->transport_server->send(
        fd, StaticResponses::YOU_ARE_LOGGED_IN); // добавить новую ошибку потом
    return;
  }

  session_manager->bind(fd, username_string);

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
