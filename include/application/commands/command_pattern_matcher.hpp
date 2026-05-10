#pragma once

#include "include/application/commands/command_pattern.hpp"

#include <string>
#include <vector>

namespace app::commands {

// Responsibility:
// - compile textual templates into tokenized patterns;
// - match an input line against a set of patterns;
// - return captures, not a command object.
//
// This class must not know about networking, serialization, or UI.
class CommandPatternMatcher {
public:
  CommandPatternMatcher() = default;

  // Add a new template, for example "/login {username}".
  void add_pattern(const std::string &command_name,
                   const std::string &template_text);

  // Match one input line against registered patterns.
  // If nothing matches, return {matched=false}.
  CommandMatch match(const std::string &input) const;

  // Clear all registered patterns.
  void clear();

private:
  std::vector<CommandPattern> patterns_;

  static std::vector<std::string> split_tokens(const std::string &text);
  static CommandPattern compile_pattern(const std::string &command_name,
                                         const std::string &template_text);
  static CommandMatch try_match(const CommandPattern &pattern,
                                const std::vector<std::string> &input_tokens);
};

} // namespace app::commands
