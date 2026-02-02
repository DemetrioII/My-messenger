#include "../../../include/services/commands_impl/join.hpp"

CommandType JoinCommand::getType() const { return CommandType::JOIN; }

void JoinCommand::fromParsedCommand(const ParsedCommand &parsed) {
  if (!parsed.args.empty()) {
    chat_name = parsed.args[0];
  }
}

Message JoinCommand::toMessage() const {
  ParsedCommand pc;
  pc.name = "join";
  pc.args.push_back(chat_name);

  Parser parser;
  return parser.make_command_from_struct(pc);
}

void JoinCommand::fromMessage(const Message &msg) {
  chat_name = msg.get_meta(1);
}

void JoinCommand::execeuteOnServer(std::shared_ptr<ServerContext> context) {
  auto user_service = context->user_service;
  auto chat_service = context->chat_service;
  auto session_manager = context->session_manager;
  auto transport_server = context->transport_server;
  auto fd = context->fd;
  auto chat_name_string = std::string(chat_name.begin(), chat_name.end());

  auto user_id = session_manager->get_username(fd);
  if (!user_id.has_value()) {
    transport_server->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }
  auto res = chat_service->add_member(chat_name_string, *user_id);

  if (!res.has_value()) {
    if (res.error() == ServiceError::ChatNotFound) {
      transport_server->send(fd, StaticResponses::CHAT_NOT_FOUND);
      return;
    }
    if (res.error() == ServiceError::AlreadyMember) {
      transport_server->send(fd, StaticResponses::YOU_ARE_ALREADY_MEMBER);
      return;
    }
  }

  std::string response = "You joined the room " + chat_name_string;
  transport_server->send(
      fd, context->serializer.serialize(
              Message(std::vector<uint8_t>(response.begin(), response.end()), 0,
                      {}, MessageType::Text)));
}

void JoinCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  std::shared_ptr<IClient> client = context->client;
  Serializer serializer;
  client->send_to_server(serializer.serialize(toMessage()));
}

JoinCommand::~JoinCommand() {}
