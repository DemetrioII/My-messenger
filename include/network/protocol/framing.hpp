#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>

class FramerMessage {

public:
  FramerMessage() = default;

  bool has_message_in_buffer(const std::vector<uint8_t> &buffer) const;

  std::vector<uint8_t> extract_message(std::vector<uint8_t> &buffer);

  void form_message(const std::vector<uint8_t> &data,
                    std::vector<uint8_t> &buffer);

  ~FramerMessage() = default;
};
