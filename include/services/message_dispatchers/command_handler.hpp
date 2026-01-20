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
  MessageCommandHandler()
      : clientCommandBus(clientCommandRegistry),
        serverCommandBus(serverCommandRegistry) {

    clientCommandRegistry.registerCommand("exit",
                                          std::make_unique<ExitCommand>());

    clientCommandRegistry.registerCommand("login",
                                          std::make_unique<LoginCommand>());

    clientCommandRegistry.registerCommand("room",
                                          std::make_unique<MakeRoomCommand>());

    clientCommandRegistry.registerCommand(
        "send", std::make_unique<SendGroupMessageCommand>());

    clientCommandRegistry.registerCommand("join",
                                          std::make_unique<JoinCommand>());

    clientCommandRegistry.registerCommand("getpub",
                                          std::make_unique<GetPubkeyCommand>());

    clientCommandRegistry.registerCommand(
        "pmess", std::make_unique<PrivateMessageCommand>());

    clientCommandRegistry.registerCommand("help",
                                          std::make_unique<HelpCommand>());

    clientCommandRegistry.registerCommand("sendfile",
                                          std::make_unique<SendFileCommand>());

    serverCommandRegistry.registerCommand("exit",
                                          std::make_unique<ExitCommand>());

    serverCommandRegistry.registerCommand("login",
                                          std::make_unique<LoginCommand>());

    serverCommandRegistry.registerCommand("room",
                                          std::make_unique<MakeRoomCommand>());

    serverCommandRegistry.registerCommand(
        "send", std::make_unique<SendGroupMessageCommand>());

    serverCommandRegistry.registerCommand("join",
                                          std::make_unique<JoinCommand>());

    serverCommandRegistry.registerCommand("getpub",
                                          std::make_unique<GetPubkeyCommand>());

    serverCommandRegistry.registerCommand(
        "pmess", std::make_unique<PrivateMessageCommand>());

    serverCommandRegistry.registerCommand("help",
                                          std::make_unique<HelpCommand>());

    serverCommandRegistry.registerCommand("sendfile",
                                          std::make_unique<SendFileCommand>());
  }

  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override {
    auto cmd_struct = parser.make_struct_from_command(msg);
    clientCommandBus.dispatch(cmd_struct, context);
  }

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override {
    serverCommandBus.dispatch(msg, context);
  }

  MessageType getMessageType() const override { return MessageType::Command; }
};
