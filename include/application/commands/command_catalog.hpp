#pragma once

#include "include/application/commands/command_pattern.hpp"

#include <vector>

namespace app::commands {

// Responsibility:
// - own the list of supported command templates;
// - describe the "language" of the client CLI;
// - not perform matching or object creation.
//
// This is the single place where command templates should be listed.
std::vector<CommandPattern> default_command_catalog();

} // namespace app::commands
