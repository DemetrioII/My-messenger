#include "include/application/commands/command_interface.hpp"

void ClientCommandRegistry::registerCommand(
    const std::string &name, std::unique_ptr<ICommandHandler> cmd) {
  commands[name] = std::move(cmd);
}

void ClientCommandRegistry::setClientContext(
    const std::shared_ptr<ClientContext> context) {
  clientContext = context;
}

ICommandHandler *ClientCommandRegistry::find(const std::string &name) const {
  auto it = commands.find(name);
  if (it != commands.end())
    return it->second.get();
  else
    return nullptr;
}

bool ClientCommandRegistry::exists(const std::string &name) const {
  return commands.find(name) != commands.end();
}

ClientCommandBus::ClientCommandBus(ClientCommandRegistry &registry_)
    : registry(registry_) {}

void ClientCommandBus::dispatch(const Message &msg,
                                const std::shared_ptr<ClientContext> context) {
  ParsedCommand cmd = context->parser.make_struct_from_command(msg);
  auto handler = registry.find(cmd.name);
  if (!handler) {
    std::cerr << "[CommandBus] Unknown command type: " << cmd.name << std::endl;
    return;
  }
  handler->fromMessage(msg);
  handler->executeOnClient(context);
}

void PeerCommandRegistry::registerCommand(
    const std::string &name, std::unique_ptr<ICommandHandler> cmd) {
  commands[name] = std::move(cmd);
}

void PeerCommandRegistry::setPeerContext(
    const std::shared_ptr<PeerContext> context) {
  peerContext = context;
}

ICommandHandler *PeerCommandRegistry::find(const std::string &name) const {
  auto it = commands.find(name);
  if (it != commands.end())
    return it->second.get();
  else
    return nullptr;
}

bool PeerCommandRegistry::exists(const std::string &name) const {
  return commands.find(name) != commands.end();
}

PeerCommandBus::PeerCommandBus(PeerCommandRegistry &registry)
    : registry_(registry) {}

void PeerCommandBus::dispatchSending(
    const Message &msg, const std::shared_ptr<PeerContext> context) {
  ParsedCommand cmd = context->parser.make_struct_from_command(msg);
  auto handler = registry_.find(cmd.name);
  if (!handler) {
    std::cerr << "[CommandBus] Unknown command type: " << cmd.name << std::endl;
    return;
  }
  handler->fromMessage(msg);
  handler->send_from_peer(context);
}

void PeerCommandBus::dispatchReceiving(
    const Message &msg, const std::shared_ptr<PeerContext> context) {
  ParsedCommand cmd = context->parser.make_struct_from_command(msg);
  auto handler = registry_.find(cmd.name);
  if (!handler) {
    std::cerr << "[CommandBus] Unknown command type: " << cmd.name << std::endl;
    return;
  }
  handler->fromMessage(msg);
  handler->recv_on_peer(context->fd, context);
}

void ServerCommandRegistry::registerCommand(
    const std::string &name, std::unique_ptr<ICommandHandler> cmd) {
  commands[name] = std::move(cmd);
}

void ServerCommandRegistry::setServerContext(
    const std::shared_ptr<ServerContext> context) {
  server_context = context;
}

ICommandHandler *ServerCommandRegistry::find(const std::string &name) const {
  auto it = commands.find(name);
  if (it != commands.end())
    return it->second.get();
  else
    return nullptr;
}

bool ServerCommandRegistry::exists(const std::string &name) const {
  return commands.find(name) != commands.end();
}

ServerCommandBus::ServerCommandBus(ServerCommandRegistry &registry_)
    : registry(registry_) {}

void ServerCommandBus::dispatch(const Message &msg,
                                std::shared_ptr<ServerContext> context) {
  ParsedCommand cmd = context->parser.make_struct_from_command(msg);
  auto handler = registry.find(cmd.name);
  if (!handler) {
    std::cerr << "[CommandBus] Unknown command type: " << cmd.name << std::endl;
    return;
  }
  handler->fromMessage(msg);
  handler->execeuteOnServer(context);
}
