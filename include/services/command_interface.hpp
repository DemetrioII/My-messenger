#pragma once
#include "../models/message.hpp"
#include "../network/protocol/parser.hpp"
#include "client_context.hpp"
#include "messageing_service.hpp"
#include <any>
#include <string>
#include <vector>

#define PREPARE_ARGS(...) std::vector<std::any> args = {__VA_ARGS__}

#define GET_ARG(type, index, name) type name = std::any_cast<type>(args[index])

#define EXECUTE_CMD_CLIENT(cmd_class, msg_obj, ...)                            \
  {                                                                            \
    PREPARE_ARGS(__VA_ARGS__)                                                  \
    cmd_class cmd;                                                             \
    cmd.fromMessage(msg_obj);                                                  \
    cmd.executeOnClient(args);                                                 \
  }

#define EXECUTE_CMD_CLIENTpc(cmd_class, pc, ...)                               \
  {                                                                            \
    PREPARE_ARGS(__VA_ARGS__);                                                 \
    cmd_class cmd;                                                             \
    cmd.fromParsedCommand(pc);                                                 \
    cmd.executeOnClient(args);                                                 \
    if (!cmd.isClientOnly()) {                                                 \
      Message msg = cmd.toMessage();                                           \
      client->send_to_server(serializer.serialize(msg));                       \
    }                                                                          \
  }

class ICommandHandler {
public:
  virtual ~ICommandHandler() = default;

  virtual void fromParsedCommand(const ParsedCommand &parsed) = 0;

  virtual Message toMessage() const = 0;

  virtual void fromMessage(const Message &msg) = 0;

  virtual void execeuteOnServer(std::shared_ptr<ServerContext> context) = 0;

  virtual void executeOnClient(std::shared_ptr<ClientContext> context) = 0;

  virtual bool isClientOnly() const { return false; }

  virtual CommandType getType() const = 0;
};

class ClientCommandRegistry {
  using CommandHandler =
      std::function<void(const std::vector<std::vector<uint8_t>> &)>;
  std::unordered_map<std::string, std::unique_ptr<ICommandHandler>> commands;

  std::shared_ptr<ClientContext> clientContext;

public:
  void registerCommand(const std::string &name,
                       std::unique_ptr<ICommandHandler> cmd) {
    commands[name] = std::move(cmd);
  }

  void setClientContext(const std::shared_ptr<ClientContext> context) {
    clientContext = context;
  }

  ICommandHandler *find(const std::string &name) const {
    auto it = commands.find(name);
    if (it != commands.end())
      return it->second.get();
    else
      return nullptr;
  }

  bool exists(const std::string &name) const {
    return commands.find(name) != commands.end();
  }
};

class ServerCommandRegistry {
  using CommandHandler = std::function<void(int, const Message &)>;
  std::unordered_map<std::string, std::unique_ptr<ICommandHandler>> commands;

  std::shared_ptr<ServerContext> server_context;

public:
  void registerCommand(const std::string &name,
                       std::unique_ptr<ICommandHandler> cmd) {
    commands[name] = std::move(cmd);
  }

  void setServerContext(const std::shared_ptr<ServerContext> context) {
    server_context = context;
  }

  ICommandHandler *find(const std::string &name) const {
    auto it = commands.find(name);
    if (it != commands.end())
      return it->second.get();
    else
      return nullptr;
  }

  bool exists(const std::string &name) const {
    return commands.find(name) != commands.end();
  }
};

class ClientCommandBus {
  ClientCommandRegistry &registry;

public:
  ClientCommandBus(ClientCommandRegistry &registry_) : registry(registry_) {}
  void dispatch(const Message &msg,
                const std::shared_ptr<ClientContext> context) {
    ParsedCommand cmd = context->parser.make_struct_from_command(msg);
    auto handler = registry.find(cmd.name);
    if (!handler) {
      std::cerr << "[CommandBus] Unknown command type: " << cmd.name
                << std::endl;
      return;
    }
    handler->fromMessage(msg);
    handler->executeOnClient(context);
  }
};

class ServerCommandBus {
  ServerCommandRegistry &registry;

public:
  ServerCommandBus(ServerCommandRegistry &registry_) : registry(registry_) {}

  void dispatch(const Message &msg, std::shared_ptr<ServerContext> context) {
    ParsedCommand cmd = context->parser.make_struct_from_command(msg);
    auto handler = registry.find(cmd.name);
    if (!handler) {
      std::cerr << "[CommandBus] Unknown command type: " << cmd.name
                << std::endl;
      return;
    }
    handler->fromMessage(msg);
    handler->execeuteOnServer(context);
  }
};
