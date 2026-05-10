#include "include/application/commands/command_factory.hpp"

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

std::unique_ptr<ICommandHandler>
app::commands::CommandFactory::create(const std::string &command_name) const {
  if (command_name == "login") {
    return std::make_unique<LoginCommand>();
  }
  if (command_name == "room") {
    return std::make_unique<MakeRoomCommand>();
  }
  if (command_name == "join") {
    return std::make_unique<JoinCommand>();
  }
  if (command_name == "send") {
    return std::make_unique<SendGroupMessageCommand>();
  }
  if (command_name == "pmess") {
    return std::make_unique<PrivateMessageCommand>();
  }
  if (command_name == "getpub") {
    return std::make_unique<GetPubkeyCommand>();
  }
  if (command_name == "sendfile") {
    return std::make_unique<SendFileCommand>();
  }
  if (command_name == "connect") {
    return std::make_unique<ConnectCommand>();
  }
  if (command_name == "disconnect") {
    return std::make_unique<DisconnectCommand>();
  }
  if (command_name == "exit") {
    return std::make_unique<ExitCommand>();
  }
  if (command_name == "help") {
    return std::make_unique<HelpCommand>();
  }
  return nullptr;
}
