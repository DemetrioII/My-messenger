#pragma once

#include "include/application/services/services_common.hpp"

class SessionManager {
public:
  void bind(int fd, const std::string &username);

  void unbind(int fd);

  ServiceResult<std::string> get_username(int fd) const;

  ServiceResult<int> get_fd(const std::string &username) const;

private:
  std::unordered_map<int, std::string> fd_to_name_;
  std::unordered_map<std::string, int> name_to_fd_;
};
