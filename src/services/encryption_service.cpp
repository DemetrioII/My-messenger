#include "../../../include/services/encryption_service.hpp"
#include <iostream>

void EncryptionService::set_key() {
  DH_key = load_or_generate_key();
  const std::string path = "identity.key";

  if (IdentityKey::file_exists(path)) {
    auto bytes = IdentityKey::load_file(path);
    identity_key = IdentityKey::from_private_bytes(bytes);
  } else {
    identity_key = IdentityKey::generate();
    IdentityKey::save_file(path, identity_key.private_bytes());
  }
}

std::vector<uint8_t> EncryptionService::sign() {
  return identity_key.sign(DH_key.public_bytes());
}

std::vector<uint8_t> EncryptionService::get_DH_bytes() {
  return DH_key.public_bytes();
}

std::vector<uint8_t> EncryptionService::get_identity_bytes() {
  return identity_key.public_bytes();
}

std::vector<uint8_t> EncryptionService::encrypt_for(
    const std::vector<uint8_t> &sender, const std::vector<uint8_t> &username,
    const std::vector<uint8_t> &plaintext, uint64_t counter) {

  if (DH_keys.find(username) == DH_keys.end())
    throw std::runtime_error("User public " +
                             std::string(username.begin(), username.end()) +
                             " key not found");
  auto shared_secret = DH_key.compute_shared_secret(DH_keys[username]);
  auto encryption_key =
      HKDF::derive_for_messaging(shared_secret, sender, username, "encryption");
  auto ciphertext =
      aes_gcm_encryptor.encrypt(encryption_key, plaintext, counter);

  if (!shared_secret.empty())
    OPENSSL_cleanse(shared_secret.data(), shared_secret.size());
  if (!encryption_key.empty())
    OPENSSL_cleanse(encryption_key.data(), encryption_key.size());

  return ciphertext;
}

std::vector<uint8_t> EncryptionService::decrypt_for(
    const std::vector<uint8_t> &sender, const std::vector<uint8_t> &username,
    const std::vector<uint8_t> &ciphertext,
    const std::vector<uint8_t> &signature, uint64_t counter) {
  if (DH_keys.find(sender) == DH_keys.end())
    throw std::runtime_error("User public " +
                             std::string(sender.begin(), sender.end()) +
                             " key not found");
  if (!identity_keys[sender].verify(DH_keys[sender].public_bytes(),
                                    signature)) {
    throw std::runtime_error("MITM detected!");
  } else {
    /* std::cout << "Verification for user "
              << std::string(sender.begin(), sender.end()) << " was successful!"
              << std::endl;
    std::cout << std::string(signature.begin(), signature.end()) << std::endl;
    auto temp = identity_keys[sender].public_bytes();
    std::cout << std::string(temp.begin(), temp.end()) << std::endl; */
  }
  auto shared_secret = DH_key.compute_shared_secret(DH_keys[sender]);
  auto decryption_key =
      HKDF::derive_for_messaging(shared_secret, sender, username, "encryption");
  auto plaintext =
      aes_gcm_encryptor.decrypt(decryption_key, ciphertext, counter);

  if (!shared_secret.empty())
    OPENSSL_cleanse(shared_secret.data(), shared_secret.size());
  if (!decryption_key.empty())
    OPENSSL_cleanse(decryption_key.data(), decryption_key.size());

  return plaintext;
}

void EncryptionService::cache_public_key(
    const std::vector<uint8_t> &username, const std::vector<uint8_t> &pubkey,
    const std::vector<uint8_t> &identity_key) {
  DH_keys[username] = DH_Key::from_public_bytes(pubkey);
  identity_keys[username] = IdentityKey::from_public_bytes(identity_key);
}
