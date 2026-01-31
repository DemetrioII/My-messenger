#include "../../../include/services/encryption_service.hpp"

void EncryptionService::set_identity_key() {
  identity_key = load_or_generate_key();
}

std::vector<uint8_t> EncryptionService::get_public_bytes() {
  return identity_key.public_bytes();
}

std::vector<uint8_t>
EncryptionService::encrypt_for(const std::vector<uint8_t> &sender,
                               const std::vector<uint8_t> &username,
                               const std::vector<uint8_t> &plaintext) {

  if (keys.find(username) == keys.end())
    throw std::runtime_error("User public " +
                             std::string(username.begin(), username.end()) +
                             " key not found");
  auto shared_secret = identity_key.compute_shared_secret(keys[username]);
  auto encryption_key =
      HKDF::derive_for_messaging(shared_secret, sender, username, "encryption");
  auto ciphertext = aes_gcm_encryptor.encrypt(encryption_key, plaintext);
  return ciphertext;
}

std::vector<uint8_t>
EncryptionService::decrypt_for(const std::vector<uint8_t> &sender,
                               const std::vector<uint8_t> &username,
                               const std::vector<uint8_t> &ciphertext) {
  if (keys.find(sender) == keys.end())
    throw std::runtime_error("User public " +
                             std::string(sender.begin(), sender.end()) +
                             " key not found");
  auto shared_secret = identity_key.compute_shared_secret(keys[sender]);
  auto decryption_key =
      HKDF::derive_for_messaging(shared_secret, sender, username, "encryption");
  auto plaintext = aes_gcm_encryptor.decrypt(decryption_key, ciphertext);
  return plaintext;
}

void EncryptionService::cache_public_key(const std::vector<uint8_t> &username,
                                         const std::vector<uint8_t> &pubkey) {
  keys[username] = IdentityKey::from_public_bytes(pubkey);
}
