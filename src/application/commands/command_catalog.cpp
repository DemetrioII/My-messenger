#include "include/application/commands/command_catalog.hpp"

std::vector<app::commands::CommandPattern>
app::commands::default_command_catalog() {
  std::vector<CommandPattern> catalog;
  catalog.push_back({"login", "/login {username}", {}});
  catalog.push_back({"room", "/room {name}", {}});
  catalog.push_back({"join", "/join {name}", {}});
  catalog.push_back({"send", "/send {chat} {message...}", {}});
  catalog.push_back({"pmess", "/pmess {recipient} {message...}", {}});
  catalog.push_back({"getpub", "/getpub {username}", {}});
  catalog.push_back({"sendfile", "/sendfile {recipient} {filename}", {}});
  catalog.push_back({"connect", "/connect {ip} {port} {dh} {identity} {sig}", {}});
  catalog.push_back({"disconnect", "/disconnect {username}", {}});
  catalog.push_back({"exit", "/exit", {}});
  catalog.push_back({"help", "/help", {}});

  return catalog;
}
