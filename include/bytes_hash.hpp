#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace std {
template <> struct hash<std::vector<uint8_t>> {
  size_t operator()(const std::vector<uint8_t> &v) const noexcept {
    size_t h = 0xcbf29ce484222325;
    for (auto b : v) {
      h ^= b;
      h *= 0x100000001b3;
    }
    return h;
  }
};
} // namespace std
