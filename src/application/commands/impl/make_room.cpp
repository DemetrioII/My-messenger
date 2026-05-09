#include "include/application/commands/impl/make_room.hpp"

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
  context->app_service->create_room(
      std::string(chat_name.begin(), chat_name.end()));
}

void MakeRoomCommand::executeOnClient(std::shared_ptr<ClientContext> context) {

  std::shared_ptr<IClient> client = context->client;
  Serializer serializer;
  client->send_to_server(serializer.serialize(toMessage()));
}

void MakeRoomCommand::send_from_peer(std::shared_ptr<PeerContext> context) {}

void MakeRoomCommand::recv_on_peer(int fd,
                                   std::shared_ptr<PeerContext> context) {}

bool MakeRoomCommand::isClientOnly() const { return false; }

MakeRoomCommand::~MakeRoomCommand() {}
