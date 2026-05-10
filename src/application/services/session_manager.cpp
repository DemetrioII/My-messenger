#include "include/application/services/session_manager.hpp"

void SessionManager::bind(int fd, const std::string &username) {
  fd_to_name_[fd] = username;
  name_to_fd_[username] = fd;
}

void SessionManager::unbind(int fd) {
  if (fd_to_name_.find(fd) == fd_to_name_.end())
    return;
  name_to_fd_.erase(fd_to_name_[fd]);
  fd_to_name_.erase(fd);
}

ServiceResult<std::string> SessionManager::get_username(int fd) const {
  if (fd_to_name_.find(fd) == fd_to_name_.end())
    return std::unexpected(ServiceError::UserNotFound);
  return fd_to_name_.find(fd)->second;
}

ServiceResult<int> SessionManager::get_fd(const std::string &username) const {
  if (name_to_fd_.find(username) == name_to_fd_.end())
    return std::unexpected(ServiceError::UserNotFound);
  return name_to_fd_.find(username)->second;
}
