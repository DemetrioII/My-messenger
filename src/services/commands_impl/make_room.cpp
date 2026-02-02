#include "../../../include/services/commands_impl/make_room.hpp"

CommandType MakeRoomCommand::getType() const { return CommandType::MAKE_ROOM; }

void MakeRoomCommand::fromParsedCommand(const ParsedCommand &parsed) {
  if (!parsed.args.empty()) {
    chat_name = parsed.args[0];
  }
}

Message MakeRoomCommand::toMessage() const {
  ParsedCommand pc;
  pc.name = "room";
  pc.args = {chat_name};

  Parser parser;
  return parser.make_command_from_struct(pc);
}

void MakeRoomCommand::fromMessage(const Message &msg) {
  chat_name = msg.get_meta(1);
}

void MakeRoomCommand::execeuteOnServer(std::shared_ptr<ServerContext> context) {
  auto user_service = context->user_service;
  auto chat_service = context->chat_service;
  auto session_manager = context->session_manager;
  auto fd = context->fd;
  auto transport_server = context->transport_server;
  auto parser = context->parser;
  auto username = session_manager->get_username(fd);
  if (username.has_value()) {
    transport_server->send(fd, StaticResponses::YOU_NEED_TO_LOGIN);
    return;
  }

  if (chat_name.empty()) {
    transport_server->send(fd, StaticResponses::EMPTY_CHAT_NAME);
    return;
  }

  std::vector<std::string> members{*username};
  std::string chat_name_str(chat_name.begin(), chat_name.end());
  auto chat_res = chat_service->create_chat(chat_name_str, *username);
  if (!chat_res.has_value()) {
    transport_server->send(fd, StaticResponses::CHAT_ALREADY_EXISTS);
    return;
  }

  std::string response_str = "Room " + chat_name_str + " was created!";

  Message response{
      std::vector<uint8_t>(response_str.begin(), response_str.end()),
      0,
      {},
      MessageType::Text};
  transport_server->send(fd, context->serializer.serialize(response));
}

void MakeRoomCommand::executeOnClient(std::shared_ptr<ClientContext> context) {

  std::shared_ptr<IClient> client = context->client;
  Serializer serializer;
  client->send_to_server(serializer.serialize(toMessage()));
}

bool MakeRoomCommand::isClientOnly() const { return false; }

MakeRoomCommand::~MakeRoomCommand() {}
