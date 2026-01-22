#pragma once
#include "../command_interface.hpp"

class GetPubkeyCommand : public ICommandHandler {
  std::vector<uint8_t> username;

public:
  CommandType getType() const override { return CommandType::GET_PUBKEY; }

  void fromParsedCommand(const ParsedCommand &parsed) override {
    if (!parsed.args.empty()) {
      username = parsed.args[0];
    }
  }

  Message toMessage() const override {
    return Message({}, 2, {{static_cast<uint8_t>(getType())}, username},
                   MessageType::Command);
  }

  void fromMessage(const Message &msg) override { username = msg.get_meta(1); }

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override {
    auto service = context->messaging_service;
    auto transport_server = context->transport_server;
    int fd = context->fd;
    if (!service->is_authenticated(fd)) {
      std::string error_msg = "[Error]: You must authenticate first";
      transport_server->send(
          fd, context->serializer.serialize(Message(
                  std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0,
                  {}, MessageType::Text)));
      return;
    }
    if (username.empty()) {
      std::string error_msg = "Usage: /getpub <username>";
      transport_server->send(
          context->fd,
          context->serializer.serialize(
              Message(std::vector<uint8_t>(error_msg.begin(), error_msg.end()),
                      0, {}, MessageType::Text)));
      return;
    }
    auto user_id_str = service->get_user_by_name(
        std::string(username.begin(), username.end()));
    if (user_id_str == "") {
      std::string error_msg = "User not found";
      transport_server->send(
          context->fd,
          context->serializer.serialize(
              Message(std::vector<uint8_t>(error_msg.begin(), error_msg.end()),
                      0, {}, MessageType::Text)));
      return;
    }
    auto &user = service->get_user_by_id(user_id_str);
    Message msg(
        {user.get_public_key(),
         2,
         {std::vector<uint8_t>{static_cast<uint8_t>(CommandType::GET_PUBKEY)},
          username},
         MessageType::Response});
    context->transport_server->send(context->fd,
                                    context->serializer.serialize(msg));
  }

  void executeOnClient(std::shared_ptr<ClientContext> context) override {
    std::shared_ptr<IClient> client = context->client;
    std::shared_ptr<Serializer> serializer = context->serializer;
    client->send_to_server(serializer->serialize(toMessage()));
  }

  ~GetPubkeyCommand() override {}
};
