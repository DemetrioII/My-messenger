#include "include/application/services/user_service.hpp"

ServiceResult<std::string>
UserService::register_user(const std::string &name,
                           const std::vector<uint8_t> &pubkey,
                           const std::vector<uint8_t> &identity_pub,
                           const std::vector<uint8_t> &signature) {
  User user_obj("", name);
  if (users_.find(name) != users_.end())
    return std::unexpected(ServiceError::AlreadyExists);
  users_[name] = std::make_unique<User>(user_obj);
  set_public_key(name, pubkey, identity_pub, signature);
  // ПОКА ЭТО НЕ НУЖНО: name_to_id_[name] = "";
  return name;
}

ServiceResult<User *> UserService::find_user(const UserIdentifier &id) {
  auto it = users_.find(id);
  if (it == users_.end())
    return std::unexpected(ServiceError::UserNotFound);
  return users_[id].get();
}

void UserService::set_public_key(const UserIdentifier &id,
                                 const std::vector<uint8_t> &pubkey,
                                 const std::vector<uint8_t> &identity_pub,
                                 const std::vector<uint8_t> &signature) {
  auto it = users_.find(id);
  if (it == users_.end())
    return;
  it->second->set_public_DH_key(pubkey);
  it->second->set_public_Identity_key(identity_pub);
  it->second->set_key_signature(signature);
}

void UserService::remove_user(const UserIdentifier &id) {
  auto it = users_.find(id);
  if (it == users_.end())
    return;
  users_.erase(it);
}
