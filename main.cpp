#include "common.hpp"
#include <cstdio>
#include <iomanip>

int main(int argc, char *argv[]) {
  Serializer serializer;
  std::string error_msg = "[Error]: You are not member of this chat";
  auto raw = serializer.serialize(
      Message(std::vector<uint8_t>(error_msg.begin(), error_msg.end()), 0, {},
              MessageType::Text));
  std::cout << "{ ";
  for (int i = 0; i < raw.size(); ++i) {
    std::cout << "0x" << std::setfill('0') << std::setw(2) << std::hex
              << (int)raw[i] << ", ";
  }
  std::cout << "};";
  std::cout << std::endl;
  start_server();
  return 0;
}
