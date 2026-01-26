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
  auto service = context->messaging_service;
  auto transport_server = context->transport_server;
  auto fd = context->fd;
  auto chat_name_string = std::string(chat_name.begin(), chat_name.end());
  if (service->chat_id_by_name(chat_name_string) == "") {
    std::string error_msg = "Chat not found";
    transport_server->send(
        fd, context->serializer.serialize(Message(
                std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0, {},
                MessageType::Text)));
    return;
  }
  auto user_id = service->get_user_id_by_fd(fd);
  if (user_id == "") {
    std::string error_msg = "Please /login first";
    transport_server->send(
        fd, context->serializer.serialize(Message(
                std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0, {},
                MessageType::Text)));
    return;
  }
  auto username = service->get_user_by_id(user_id).get_username();
  if (service->is_member_of_chat(chat_name_string, user_id)) {
    std::string error_msg = "[Error]: You are already member of this chat";
    transport_server->send(
        fd, context->serializer.serialize(Message(
                std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0, {},
                MessageType::Text)));
    return;
  }
  service->join_chat_by_name(user_id, chat_name_string);

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
