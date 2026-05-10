#pragma once

#include "include/application/commands/command_pattern.hpp"
#include "include/application/commands/command_interface.hpp"

#include <memory>

namespace app::commands {

// Responsibility:
// - take a matched command name + captures;
// - construct the concrete command object;
// - keep command creation out of parsing/matching code.
//
// This class must not know how commands are matched or serialized.
class CommandFactory {
public:
  CommandFactory() = default;

  // Create a command by its matched name.
  // Returns nullptr if the command is unknown.
  std::unique_ptr<ICommandHandler>
  create(const std::string &command_name) const;
};

} // namespace app::commands
