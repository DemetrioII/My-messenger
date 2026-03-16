// parser.hpp

#pragma once
#include "../../models/chat.hpp"
#include "../../models/message.hpp"
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
  const std::unordered_map<std::string, int> CMD_TABLE = {
      {"login", 2},   {"room", 2},       {"join", 2},     {"send", 2},
      {"connect", 6}, {"disconnect", 3}, {"pmess", 2},    {"getpub", 4},
      {"getid", 1},   {"exit", 1},       {"sendfile", 3}, {"", 0}};

  const std::unordered_map<CommandType, std::string> command_names = {
      {CommandType::LOGIN, "login"},
      {CommandType::MAKE_ROOM, "room"},
      {CommandType::JOIN, "join"},
      {CommandType::SEND, "send"},
      {CommandType::PRIVATE_MESSAGE, "pmess"},
      {CommandType::GET_PUBKEY, "getpub"},
      {CommandType::SEND_FILE, "sendfile"},
      {CommandType::CONNECT, "connect"},
      {CommandType::DISCONNECT, "disconnect"},
      {CommandType::EXIT, "exit"}};

  const std::unordered_map<std::string, CommandType> command_types = {
      {"login", CommandType::LOGIN},
      {"room", CommandType::MAKE_ROOM},
      {"join", CommandType::JOIN},
      {"send", CommandType::SEND},
      {"pmess", CommandType::PRIVATE_MESSAGE},
      {"getpub", CommandType::GET_PUBKEY},
      {"sendfile", CommandType::SEND_FILE},
      {"connect", CommandType::CONNECT},
      {"disconnect", CommandType::DISCONNECT},
      {"exit", CommandType::EXIT}};

public:
  Parser() = default;

  Message parse(const std::string &message);

  Message make_command_from_struct(const ParsedCommand &cmd_struct);

  ParsedCommand parse_line(const std::string &line);

  ParsedCommand make_struct_from_command(const Message &msg);

private:
};
