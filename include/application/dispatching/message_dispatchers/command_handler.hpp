#pragma once
#include "include/application/commands/command_interface.hpp"
#include "include/application/commands/impl/connect.hpp"
#include "include/application/commands/impl/disconnect.hpp"
#include "include/application/commands/impl/exit.hpp"
#include "include/application/commands/impl/get_pubkey.hpp"
#include "include/application/commands/impl/help_command.hpp"
#include "include/application/commands/impl/join.hpp"
#include "include/application/commands/impl/login.hpp"
#include "include/application/commands/impl/make_room.hpp"
#include "include/application/commands/impl/private_message.hpp"
#include "include/application/commands/impl/send_group_message.hpp"
#include "include/application/commands/impl/sendfile.hpp"
#include "include/application/dispatching/message_dispatcher.hpp"

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
