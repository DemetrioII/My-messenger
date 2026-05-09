#include "../models/user.hpp"
#include <expected>

enum class ServiceError {
  UserNotFound,
  ChatNotFound,
  AccessDenied,
  InvalidToken,
  AlreadyExists,
  AlreadyMember
};

template <typename T> using ServiceResult = std::expected<T, ServiceError>;

class SessionManager {
public:
  void bind(int fd, const std::string &username);

  void unbind(int fd);
};
