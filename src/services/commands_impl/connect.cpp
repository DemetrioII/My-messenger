#include "../../../include/services/commands_impl/connect.hpp"

CommandType ConnectCommand::getType() const { return CommandType::CONNECT; }

void ConnectCommand::fromParsedCommand(const ParsedCommand &pc) {}

Message ConnectCommand::toMessage() const {
  auto port_str = std::to_string(port);
  return Message(username, 6,
                 std::vector<std::vector<uint8_t>>{
                     {std::vector<uint8_t>{static_cast<uint8_t>(getType())},
                      std::vector<uint8_t>(ip.begin(), ip.end()),
                      std::vector<uint8_t>(port_str.begin(), port_str.end()),
                      DH_bytes, identity_bytes, signature}},
                 MessageType::Command);
}

void ConnectCommand::fromMessage(const Message &msg) {
  ip = std::string(msg.get_meta(1).begin(), msg.get_meta(1).end());
  port = stoi(std::string(msg.get_meta(2).begin(), msg.get_meta(2).end()));
  DH_bytes = msg.get_meta(3);
  identity_bytes = msg.get_meta(4);
  signature = msg.get_meta(5);
  username = msg.get_payload();
}

void ConnectCommand::execeuteOnServer(std::shared_ptr<ServerContext> context) {}

void ConnectCommand::executeOnClient(std::shared_ptr<ClientContext> context) {}

void ConnectCommand::send_from_peer(std::shared_ptr<PeerContext> context) {
  if (context->my_username.empty()) {
    std::cerr << "Sorry, but you need to login first" << std::endl;
    return;
  }
  username = context->my_username;

  int _fd = connect_to_peer(*context->peer_node, ip, port);
  std::cout << "Connecting to peer " << ip << ":" << port << std::endl;

  if (!_fd) {
    std::cerr << "Sorry, error during the connecting. Maybe peer was not found"
              << std::endl;
    return;
  }
  DH_bytes = context->encryption_service->get_DH_bytes();
  identity_bytes = context->encryption_service->get_identity_bytes();
  signature = context->encryption_service->sign();
  send_to_peer(*context->peer_node, _fd,
               context->serializer->serialize(toMessage()));
}

void ConnectCommand::recv_on_peer(int fd,
                                  std::shared_ptr<PeerContext> context) {
  auto res = context->user_service->register_user(
      std::string(username.begin(), username.end()), DH_bytes, identity_bytes,
      signature);
  if (!res.has_value()) {
    if (res.error() == ServiceError::AlreadyExists) {
      std::cerr << "This user is already connected" << std::endl;
      return;
    }
  }

  context->session_manager->bind(fd,
                                 std::string(username.begin(), username.end()));
  std::cout << "User " << std::string(username.begin(), username.end())
            << " was successfully connected" << std::endl;
  context->encryption_service->cache_public_key(username, DH_bytes,
                                                identity_bytes);
  send_to_peer(*context->peer_node, fd,
               context->serializer->serialize(
                   {context->my_username,
                    4,
                    {{static_cast<uint8_t>(getType())},
                     context->encryption_service->get_DH_bytes(),
                     context->encryption_service->get_identity_bytes(),
                     context->encryption_service->sign()},
                    MessageType::Response}));
}

ConnectCommand::~ConnectCommand() {}
