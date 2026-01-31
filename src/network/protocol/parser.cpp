#include "../../../include/network/protocol/parser.hpp"

Message Parser::parse(const std::string &message) {
  Message msg;

  if (message.empty())
    return msg;

  std::vector<uint8_t> bytes;
  if (message[0] == '/') {
    ParsedCommand cmd_struct = parse_line(message);
    msg = make_command_from_struct(cmd_struct);
  } else if (message[0] == '$') {

  } else {
    msg.header.type = MessageType::Text;
    msg.payload.assign(message.begin(), message.end());
    msg.metalen = 0;
    msg.metadata.clear();
    msg.header.length = msg.payload.size() + 1;
    msg.header.timestamp = std::chrono::time_point_cast<std::chrono::seconds>(
                               std::chrono::system_clock::now())
                               .time_since_epoch()
                               .count();
  }
  return msg;
}

Message Parser::make_command_from_struct(const ParsedCommand &cmd_struct) {
  Message msg;
  msg.header.type = MessageType::Command;
  CommandType command_type;
  /*if (cmd_struct.name == "login")
    command_type = CommandType::LOGIN;
  else if (cmd_struct.name == "pmess")
    command_type = CommandType::PRIVATE_MESSAGE;
  else if (cmd_struct.name == "getid")
    command_type = CommandType::GET_ID;
  else if (cmd_struct.name == "getpub")
    command_type = CommandType::GET_PUBKEY;
  else if (cmd_struct.name == "join")
    command_type = CommandType::JOIN;
  else if (cmd_struct.name == "room")
    command_type = CommandType::MAKE_ROOM;
  else if (cmd_struct.name == "exit")
    command_type = CommandType::EXIT;
  else if (cmd_struct.name == "sendfile")
    command_type = CommandType::SEND_FILE;
  else if (cmd_struct.name == "send")
    command_type = CommandType::SEND; */
  if (command_types.find(cmd_struct.name) != command_types.end())
    command_type = command_types.find(cmd_struct.name)->second;
  else
    command_type = CommandType::UNKNOWN;
  msg.metadata.push_back({static_cast<uint8_t>(command_type)});
  for (int i = 0; i < (int)cmd_struct.args.size(); ++i) {
    msg.metadata.push_back(cmd_struct.args[i]);
  }
  msg.metalen = msg.metadata.size();
  msg.payload.clear();
  if (!cmd_struct.args.empty()) {
    msg.payload = std::vector<uint8_t>(cmd_struct.args.back());
  }
  msg.header.length = msg.payload.size();
  for (auto &m : msg.metadata)
    msg.header.length += m.size();
  msg.header.timestamp = std::chrono::time_point_cast<std::chrono::seconds>(
                             std::chrono::system_clock::now())
                             .time_since_epoch()
                             .count();
  return msg;
}

ParsedCommand Parser::parse_line(const std::string &line) {
  ParsedCommand result;
  if (line.empty())
    return result;

  std::stringstream ss(line);
  std::string first_segment;

  // Забираем первую часть — имя команды
  if (!(ss >> first_segment))
    return result;

  // Убираем префиксы вроде '/' или '$'
  if (!first_segment.empty() &&
      (first_segment[0] == '/' || first_segment[0] == '$')) {
    result.name = first_segment.substr(1);
  } else {
    result.name = first_segment;
  }

  // Сколько аргументов ожидать
  int expected_args = 0;
  auto it = CMD_TABLE.find(result.name);
  if (it != CMD_TABLE.end())
    expected_args = it->second - 1;

  // Считываем ожидаемые аргументы
  for (int i = 0; i < expected_args; ++i) {
    std::string arg;
    if (!(ss >> arg))
      break;
    result.args.push_back(std::vector<uint8_t>(arg.begin(), arg.end()));
  }

  // Если остался хвост строки — это "длинный" последний аргумент
  if (ss.good()) {
    std::string tail;
    std::getline(ss, tail);
    size_t start = tail.find_first_not_of(" \t");
    if (start != std::string::npos) {
      size_t end = tail.find_last_not_of(" \t");
      tail = (tail.substr(start, end - start + 1));
      result.args.push_back(std::vector<uint8_t>(tail.begin(), tail.end()));
    }
  }

  return result;
}

ParsedCommand Parser::make_struct_from_command(const Message &msg) {
  ParsedCommand res;
  auto cmd_type = static_cast<CommandType>(msg.get_meta(0)[0]);
  if (command_names.find(cmd_type) != command_names.end())
    res.name = command_names.find(cmd_type)->second;
  else
    res.name = "unknown";

  for (size_t i = 1; i < msg.metalen; ++i) {
    res.args.push_back(msg.get_meta(i));
  }
  return res;
}
