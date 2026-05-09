#include "../../../include/services/message_dispatchers/command_handler.hpp"

ClientCommandHandler::ClientCommandHandler() : registry_(), bus_(registry_) {

  registry_.registerCommand("exit", std::make_unique<ExitCommand>());

  registry_.registerCommand("login", std::make_unique<LoginCommand>());

  registry_.registerCommand("room", std::make_unique<MakeRoomCommand>());

  registry_.registerCommand("send",
                            std::make_unique<SendGroupMessageCommand>());

  registry_.registerCommand("join", std::make_unique<JoinCommand>());

  registry_.registerCommand("getpub", std::make_unique<GetPubkeyCommand>());

  registry_.registerCommand("pmess", std::make_unique<PrivateMessageCommand>());

  registry_.registerCommand("help", std::make_unique<HelpCommand>());

  registry_.registerCommand("sendfile", std::make_unique<SendFileCommand>());
}

void ClientCommandHandler::handleOutgoing(
    const Message &msg, std::shared_ptr<ClientContext> context) {
  bus_.dispatch(msg, context);
}

void ClientCommandHandler::handleIncoming(
    const Message &msg, std::shared_ptr<ClientContext> context) {}

ClientCommandHandler::~ClientCommandHandler() {}

ServerCommandHandler::ServerCommandHandler() : registry_(), bus_(registry_) {
  registry_.registerCommand("exit", std::make_unique<ExitCommand>());
  registry_.registerCommand("login", std::make_unique<LoginCommand>());
  registry_.registerCommand("room", std::make_unique<MakeRoomCommand>());
  registry_.registerCommand("send",
                            std::make_unique<SendGroupMessageCommand>());
  registry_.registerCommand("join", std::make_unique<JoinCommand>());
  registry_.registerCommand("getpub", std::make_unique<GetPubkeyCommand>());
  registry_.registerCommand("help", std::make_unique<HelpCommand>());
  registry_.registerCommand("sendfile", std::make_unique<SendFileCommand>());
}

void ServerCommandHandler::handleMessage(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  bus_.dispatch(msg, context);
}

ServerCommandHandler::~ServerCommandHandler() {}

PeerCommandHandler::PeerCommandHandler() : registry_(), bus_(registry_) {
  registry_.registerCommand("exit", std::make_unique<ExitCommand>());
  registry_.registerCommand("connect", std::make_unique<ConnectCommand>());
  registry_.registerCommand("disconnect", std::make_unique<DisconnectCommand>());
  registry_.registerCommand("pmess", std::make_unique<PrivateMessageCommand>());
  registry_.registerCommand("login", std::make_unique<LoginCommand>());
}

void PeerCommandHandler::handleSending(
    const Message &msg, std::shared_ptr<PeerContext> context) {
  bus_.dispatchSending(msg, context);
}

void PeerCommandHandler::handleReceiving(
    const Message &msg, std::shared_ptr<PeerContext> context) {
  bus_.dispatchReceiving(msg, context);
}

PeerCommandHandler::~PeerCommandHandler() {}
