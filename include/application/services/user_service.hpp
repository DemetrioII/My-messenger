#pragma once

#include "include/application/services/services_common.hpp"
#include "include/models/user.hpp"

class UserService {
public:
  ServiceResult<std::string>
  register_user(const std::string &name, const std::vector<uint8_t> &DH_pubkey,
                const std::vector<uint8_t> &identity_pub,
                const std::vector<uint8_t> &signature);

  ServiceResult<User *> find_user(const UserIdentifier &id);

  void set_public_key(const UserIdentifier &id, const std::vector<uint8_t> &key,
                      const std::vector<uint8_t> &identity_pub,
                      const std::vector<uint8_t> &signature);

  void remove_user(const UserIdentifier &id);

private:
  std::unordered_map<std::string, std::unique_ptr<User>> users_;
  std::unordered_map<std::string, std::string> name_to_id_;
};
