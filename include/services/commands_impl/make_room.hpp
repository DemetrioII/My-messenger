#pragma once
#include "../command_interface.hpp"

class MakeRoomCommand : public ICommandHandler {
  std::vector<uint8_t> chat_name;

public:
  CommandType getType() const override { return CommandType::MAKE_ROOM; }

  void fromParsedCommand(const ParsedCommand &parsed) override {
    if (!parsed.args.empty()) {
      chat_name = parsed.args[0];
    }
  }

  Message toMessage() const override {
    ParsedCommand pc;
    pc.name = "room";
    pc.args = {chat_name};

    Parser parser;
    return parser.make_command_from_struct(pc);
  }

  void fromMessage(const Message &msg) override { chat_name = msg.get_meta(1); }

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override {
    auto service = context->messaging_service;
    auto fd = context->fd;
    auto transport_server = context->transport_server;
    auto parser = context->parser;
    if (!service->is_authenticated(fd)) {
      std::string error_msg = "[Error]: You need to authenticate first";
      transport_server->send(
          fd, context->serializer.serialize(Message(
                  std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0,
                  {}, MessageType::Text)));
      return;
    }

    if (chat_name.empty()) {
      std::string error_msg = "[Error]: Please enter non-empty chat name";
      transport_server->send(
          fd, context->serializer.serialize(Message(
                  std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0,
                  {}, MessageType::Text)));
      return;
    }

    std::string user_id = service->get_user_id_by_fd(fd);

    std::vector<std::string> members{user_id};
    std::string chat_name_str(chat_name.begin(), chat_name.end());
    if (!service->get_chat_id_by_name(chat_name_str).empty()) {
      std::string error_msg =
          "[Error]: Chat " + chat_name_str + " is already exists";
      transport_server->send(
          fd, context->serializer.serialize(Message(
                  std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0,
                  {}, MessageType::Text)));
      return;
    }
    service->create_chat(user_id, chat_name_str, ChatType::Group, members);

    std::string response_str = "Room " + chat_name_str + " was created!";

    Message response{
        std::vector<uint8_t>(response_str.begin(), response_str.end()),
        0,
        {},
        MessageType::Text};
    transport_server->send(fd, context->serializer.serialize(response));
  }

  void executeOnClient(std::shared_ptr<ClientContext> context) override {

    std::shared_ptr<IClient> client = context->client;
    Serializer serializer;
    client->send_to_server(serializer.serialize(toMessage()));
  }

  bool isClientOnly() const override { return false; }

  ~MakeRoomCommand() override {}
};
