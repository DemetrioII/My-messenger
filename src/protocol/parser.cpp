#include "include/protocol/parser.hpp"

namespace {
uint64_t now_seconds() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

const std::unordered_map<std::string, CommandType> &command_types_map() {
  static const std::unordered_map<std::string, CommandType> types = {
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
  return types;
}

const std::unordered_map<CommandType, std::string> &command_names_map() {
  static const std::unordered_map<CommandType, std::string> names = {
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
  return names;
}
} // namespace

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

  const auto &types = command_types_map();
  const auto it = types.find(cmd_struct.name);
  const auto command_type = it != types.end() ? it->second : CommandType::UNKNOWN;

  msg.insert_metadata(std::vector<uint8_t>{static_cast<uint8_t>(command_type)});
  for (const auto &arg : cmd_struct.args) {
    msg.insert_metadata(arg);
  }

  if (!cmd_struct.args.empty()) {
    msg.set_payload(cmd_struct.args.back());
  } else {
    msg.set_payload({});
  }
  return msg;
}

ParsedCommand Parser::parse_line(const std::string &line) {
  ParsedCommand result;
  if (line.empty()) {
    return result;
  }

  std::stringstream ss(line);
  std::string first_segment;
  if (!(ss >> first_segment)) {
    return result;
  }

  if (!first_segment.empty() &&
      (first_segment[0] == '/' || first_segment[0] == '$')) {
    result.name = first_segment.substr(1);
  } else {
    result.name = first_segment;
  }

  std::vector<std::string> tokens;
  std::string token;
  while (ss >> token) {
    tokens.push_back(token);
  }

  if (tokens.empty()) {
    return result;
  }

  // Keep compatibility with the old "last argument is the tail" behavior.
  for (size_t i = 0; i + 1 < tokens.size(); ++i) {
    result.args.emplace_back(tokens[i].begin(), tokens[i].end());
  }

  const auto tail = tokens.back();
  result.args.emplace_back(tail.begin(), tail.end());
  return result;
}

ParsedCommand Parser::make_struct_from_command(const Message &msg) {
  ParsedCommand res;
  const auto &type_meta = msg.get_meta(0);
  if (type_meta.empty()) {
    res.name = "unknown";
    return res;
  }

  const auto cmd_type = static_cast<CommandType>(type_meta[0]);
  const auto &names = command_names_map();
  const auto it = names.find(cmd_type);
  res.name = it != names.end() ? it->second : "unknown";

  for (size_t i = 1; i < msg.meta_count(); ++i) {
    res.args.push_back(msg.get_meta(i));
  }
  return res;
}
