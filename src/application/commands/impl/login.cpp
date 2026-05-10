#include "include/application/commands/impl/login.hpp"

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
  auto username_string = std::string(username.begin(), username.end());
  context->app_service->login(username_string, DH_public_bytes,
                              identity_pub_bytes, signature);
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
            << std::string_view(reinterpret_cast<const char *>(username.data()),
                                username.size())
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

void LoginCommand::send_from_peer(std::shared_ptr<PeerContext> context) {
  DH_public_bytes = context->encryption_service->get_DH_bytes();
  identity_pub_bytes = context->encryption_service->get_identity_bytes();
  signature = context->encryption_service->sign();
  context->my_username = username;
  std::cout << "You have logged in as "
            << std::string_view(reinterpret_cast<const char *>(username.data()),
                                username.size())
            << std::endl;
  // broadcast(*context->peer_node,
  // context->serializer->serialize(toMessage()));
}

LoginCommand::~LoginCommand() {}
