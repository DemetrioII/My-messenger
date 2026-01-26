#pragma once
#include "../command_interface.hpp"
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

class MessageCommandHandler : public IMessageHandler {

  ClientCommandRegistry clientCommandRegistry;
  ClientCommandBus clientCommandBus;

  ServerCommandBus serverCommandBus;
  ServerCommandRegistry serverCommandRegistry;
  Parser parser;

public:
  MessageCommandHandler();

  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override;

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override;

  MessageType getMessageType() const override;
};
