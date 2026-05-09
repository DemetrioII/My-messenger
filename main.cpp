#include "include/app/runtime.hpp"
#include "include/models/message.hpp"
#include <cstdio>
#include <iomanip>
#include <iostream>

int main(int argc, char *argv[]) {
  Serializer serializer;
  std::string error_msg = "[Error]: File not found";
  auto raw = serializer.serialize(Message({}, 0, {}, MessageType::Text));
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
