// parser.hpp

#pragma once
#include "include/models/chat.hpp"
#include "include/models/message.hpp"
#include "include/application/commands/command_catalog.hpp"
#include "include/application/commands/command_pattern_matcher.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct ParsedCommand {
  std::string name;
  std::vector<std::vector<uint8_t>> args;
};

class Parser {
  app::commands::CommandPatternMatcher command_matcher_;

public:
  Parser();

  Message parse(const std::string &message);

  Message make_command_from_struct(const ParsedCommand &cmd_struct);

  ParsedCommand parse_line(const std::string &line);

  ParsedCommand make_struct_from_command(const Message &msg);

private:
};
