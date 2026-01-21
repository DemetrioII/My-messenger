#pragma once
#include "../encryption/AESGCMEncryption.hpp"
#include "../encryption/KDF.hpp"
#include "../encryption/identity_key.hpp"
#include "../models/message.hpp"

class EncryptionService {
  IdentityKey identity_key;

  AESGCMEncryptor aes_gcm_encryptor;

  std::unordered_map<std::vector<uint8_t>, IdentityKey> keys;

  auto load_or_generate_key() { return IdentityKey::generate(); }

public:
  void set_identity_key() { identity_key = load_or_generate_key(); }

  std::vector<uint8_t> get_public_bytes() {
    return identity_key.public_bytes();
  }

  std::vector<uint8_t> encrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &plaintext) {

    // auto encryption_key = HKDF::derive_for_messaging(
    // shared_secret, userid, msg_to_send.recipient_id, "encryption");
    if (keys.find(username) == keys.end())
      throw std::runtime_error("User public key not found");
    auto shared_secret = identity_key.compute_shared_secret(keys[username]);
    auto encryption_key = HKDF::derive_for_messaging(shared_secret, sender,
                                                     username, "encryption");
    auto ciphertext = aes_gcm_encryptor.encrypt(encryption_key, plaintext);
    return ciphertext;
  }

  std::vector<uint8_t> decrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &ciphertext) {
    if (keys.find(username) == keys.end())
      throw std::runtime_error("User public key not found");
    auto shared_secret = identity_key.compute_shared_secret(keys[username]);
    auto decryption_key = HKDF::derive_for_messaging(shared_secret, sender,
                                                     username, "encryption");
    auto plaintext = aes_gcm_encryptor.decrypt(decryption_key, ciphertext);
    return plaintext;
  }

  void cache_public_key(const std::vector<uint8_t> &username,
                        const std::vector<uint8_t> &pubkey) {
    keys[username] = IdentityKey::from_public_bytes(pubkey);
  }
};
