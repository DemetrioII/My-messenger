#include "include/application/commands/impl/join.hpp"

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
  auto chat_name_string = std::string(chat_name.begin(), chat_name.end());
  context->app_service->join_room(chat_name_string);
}

void JoinCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  std::shared_ptr<IClient> client = context->client;
  Serializer serializer;
  client->send_to_server(serializer.serialize(toMessage()));
}

void JoinCommand::send_from_peer(std::shared_ptr<PeerContext> context) {}

void JoinCommand::recv_on_peer(int fd, std::shared_ptr<PeerContext> context) {}

JoinCommand::~JoinCommand() {}
