#include "include/protocol/parser.hpp"

#include <chrono>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {
uint64_t now_seconds() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

std::vector<std::string> split_ws(const std::string &line) {
  std::stringstream ss(line);
  std::vector<std::string> tokens;
  std::string token;
  while (ss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

CommandType command_type_from_name(const std::string &name) {
  if (name == "login")
    return CommandType::LOGIN;
  if (name == "room")
    return CommandType::MAKE_ROOM;
  if (name == "join")
    return CommandType::JOIN;
  if (name == "send")
    return CommandType::SEND;
  if (name == "pmess")
    return CommandType::PRIVATE_MESSAGE;
  if (name == "getpub")
    return CommandType::GET_PUBKEY;
  if (name == "sendfile")
    return CommandType::SEND_FILE;
  if (name == "connect")
    return CommandType::CONNECT;
  if (name == "disconnect")
    return CommandType::DISCONNECT;
  if (name == "exit")
    return CommandType::EXIT;
  if (name == "help")
    return CommandType::HELP;
  return CommandType::UNKNOWN;
}

std::string command_name_from_type(CommandType type) {
  switch (type) {
  case CommandType::LOGIN:
    return "login";
  case CommandType::MAKE_ROOM:
    return "room";
  case CommandType::JOIN:
    return "join";
  case CommandType::SEND:
    return "send";
  case CommandType::PRIVATE_MESSAGE:
    return "pmess";
  case CommandType::GET_PUBKEY:
    return "getpub";
  case CommandType::SEND_FILE:
    return "sendfile";
  case CommandType::CONNECT:
    return "connect";
  case CommandType::DISCONNECT:
    return "disconnect";
  case CommandType::EXIT:
    return "exit";
  case CommandType::HELP:
    return "help";
  default:
    return "unknown";
  }
}
} // namespace

Parser::Parser() {
  for (const auto &pattern : app::commands::default_command_catalog()) {
    command_matcher_.add_pattern(pattern.command_name, pattern.template_text);
  }
}

Message Parser::parse(const std::string &message) {
  if (message.empty()) {
    return {};
  }

  if (message.front() == '/') {
    return make_command_from_struct(parse_line(message));
  }

  Message msg;
  msg.header.type = MessageType::Text;
  msg.header.protocol_version = 1;
  msg.header.timestamp = now_seconds();
  msg.set_payload(std::vector<uint8_t>(message.begin(), message.end()));
  return msg;
}

Message Parser::make_command_from_struct(const ParsedCommand &cmd_struct) {
  Message msg;
  msg.header.type = MessageType::Command;
  msg.header.protocol_version = 1;
  msg.header.timestamp = now_seconds();

  const auto cmd_type = command_type_from_name(cmd_struct.name);
  msg.insert_metadata(std::vector<uint8_t>{static_cast<uint8_t>(cmd_type)});
  for (const auto &arg : cmd_struct.args) {
    msg.insert_metadata(arg);
  }

  if (!cmd_struct.args.empty()) {
    msg.set_payload(cmd_struct.args.back());
  }
  return msg;
}

ParsedCommand Parser::parse_line(const std::string &line) {
  ParsedCommand result;
  if (line.empty()) {
    return result;
  }

  const auto match = command_matcher_.match(line);
  if (match.matched) {
    result.name = match.command_name;
    result.args.reserve(match.captures.size());
    for (const auto &capture : match.captures) {
      result.args.emplace_back(capture.begin(), capture.end());
    }
    return result;
  }

  auto tokens = split_ws(line);
  if (tokens.empty()) {
    return result;
  }

  std::string head = tokens.front();
  if (!head.empty() && (head.front() == '/' || head.front() == '$')) {
    head.erase(head.begin());
  }
  result.name = head;

  for (size_t i = 1; i < tokens.size(); ++i) {
    result.args.emplace_back(tokens[i].begin(), tokens[i].end());
  }
  return result;
}

ParsedCommand Parser::make_struct_from_command(const Message &msg) {
  ParsedCommand res;
  const auto &type_meta = msg.get_meta(0);
  if (type_meta.empty()) {
    res.name = "unknown";
    return res;
  }

  res.name = command_name_from_type(static_cast<CommandType>(type_meta[0]));
  for (size_t i = 1; i < msg.meta_count(); ++i) {
    res.args.push_back(msg.get_meta(i));
  }
  return res;
}
