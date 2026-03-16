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
  pc.args.push_back(DH_public_bytes);
  pc.args.push_back(identity_pub_bytes);
  pc.args.push_back(signature);

  Parser parser;
  return parser.make_command_from_struct(pc);
}

void LoginCommand::fromMessage(const Message &msg) {
  username = msg.get_meta(1);
  DH_public_bytes = msg.get_meta(2);
  identity_pub_bytes = msg.get_meta(3);
  signature = msg.get_meta(4);
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

  auto username_res = user_service->register_user(
      username_string, DH_public_bytes, identity_pub_bytes, signature);
  if (!username_res.has_value()) {
    context->transport_server->send(fd,
                                    StaticResponses::PUBLIC_KEY_HAS_NOT_SET);
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
  DH_public_bytes = encryptionService->get_DH_bytes();
  identity_pub_bytes = encryptionService->get_identity_bytes();
  signature = encryptionService->sign();
  context->my_username = username;

  Serializer serializer;
  client->send_to_server(serializer.serialize(toMessage()));
}

void LoginCommand::recv_on_peer(int fd, std::shared_ptr<PeerContext> context) {
  std::cout << "Peer with file descriptor " << fd << " has logged in as "
            << std::string(username.begin(), username.end()) << std::endl;
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

void LoginCommand::send_from_peer(int fd,
                                  std::shared_ptr<PeerContext> context) {
  DH_public_bytes = context->encryption_service->get_DH_bytes();
  identity_pub_bytes = context->encryption_service->get_identity_bytes();
  signature = context->encryption_service->sign();
  context->my_username = username;
  std::cout << "You have logged in as "
            << std::string(username.begin(), username.end()) << std::endl;
  // broadcast(*context->peer_node,
  // context->serializer->serialize(toMessage()));
}

LoginCommand::~LoginCommand() {}
