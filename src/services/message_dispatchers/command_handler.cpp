#include "../../../include/services/message_dispatchers/command_handler.hpp"

MessageCommandHandler::MessageCommandHandler()
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

void MessageCommandHandler::handleMessageOnClient(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  auto cmd_struct = parser.make_struct_from_command(msg);
  clientCommandBus.dispatch(msg, context);
}

void MessageCommandHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  serverCommandBus.dispatch(msg, context);
}

MessageType MessageCommandHandler::getMessageType() const {
  return MessageType::Command;
}
