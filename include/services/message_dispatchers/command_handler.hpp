#pragma once
#include "../command_interface.hpp"
#include "../commands_impl/connect.hpp"
#include "../commands_impl/disconnect.hpp"
#include "../commands_impl/exit.hpp"
#include "../commands_impl/get_pubkey.hpp"
#include "../commands_impl/help_command.hpp"
#include "../commands_impl/join.hpp"
#include "../commands_impl/login.hpp"
#include "../commands_impl/make_room.hpp"
#include "../commands_impl/private_message.hpp"
#include "../commands_impl/send_group_message.hpp"
#include "../commands_impl/sendfile.hpp"
#include "../message_dispatcher.hpp"

class ClientCommandHandler : public IClientMessageHandler {
  ClientCommandRegistry registry_;
  ClientCommandBus bus_;

public:
  ClientCommandHandler();

  void handleOutgoing(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  void handleIncoming(const Message &msg,
                      const std::shared_ptr<ClientContext> context) override;

  MessageType getMessageType() const override { return MessageType::Command; }

  ~ClientCommandHandler() override;
};

class ServerCommandHandler : public IServerMessageHandler {
  ServerCommandRegistry registry_;
  ServerCommandBus bus_;

public:
  ServerCommandHandler();

  void handleMessage(const Message &msg,
                     std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override { return MessageType::Command; }

  ~ServerCommandHandler() override;
};

class PeerCommandHandler : public IPeerMessageHandler {
  PeerCommandRegistry registry_;
  PeerCommandBus bus_;

public:
  PeerCommandHandler();

  void handleSending(const Message &msg,
                     std::shared_ptr<PeerContext> context) override;

  void handleReceiving(const Message &msg,
                       std::shared_ptr<PeerContext> context) override;

  MessageType getMessageType() const override { return MessageType::Command; }

  ~PeerCommandHandler() override;
};
