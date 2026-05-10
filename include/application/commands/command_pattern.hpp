#pragma once

#include <string>
#include <vector>

namespace app::commands {

// One token in a command template.
// Examples:
// - literal: "/login"
// - capture: "{username}"
// - tail capture: "{message...}"
enum class PatternTokenKind {
  Literal,
  Capture,
  TailCapture,
};

struct PatternToken {
  PatternTokenKind kind;
  std::string value;
};

// A compiled command template.
// This should be created once at startup and reused for all matching.
struct CommandPattern {
  std::string command_name;
  std::string template_text;
  std::vector<PatternToken> tokens;
};

struct CommandMatch {
  bool matched = false;
  std::string command_name;
  std::vector<std::string> captures;
};

} // namespace app::commands
