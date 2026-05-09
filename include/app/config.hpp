#pragma once

#include <cstdint>
#include <string>

struct AppConfig {
  std::string server_host = "127.0.0.1";
  std::uint16_t server_port = 8080;
};
