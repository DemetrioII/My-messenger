#pragma once
#include "include/bytes_hash.hpp"
#include "include/infrastructure/crypto/AESGCMEncryption.hpp"
#include "include/infrastructure/crypto/KDF.hpp"
#include "include/infrastructure/crypto/identity_key.hpp"
#include "include/models/message.hpp"

class EncryptionService {
  DH_Key DH_key;
  IdentityKey identity_key;

  AESGCMEncryptor aes_gcm_encryptor;

  std::unordered_map<std::vector<uint8_t>, DH_Key> DH_keys;
  std::unordered_map<std::vector<uint8_t>, IdentityKey> identity_keys;

  auto load_or_generate_key() { return DH_Key::generate(); }

public:
  void set_key();

  std::vector<uint8_t> get_DH_bytes();

  std::vector<uint8_t> get_identity_bytes();

  std::vector<uint8_t> sign();

  std::vector<uint8_t> encrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &plaintext,
                                   uint64_t counter);

  std::vector<uint8_t> decrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &ciphertext,
                                   const std::vector<uint8_t> &signature,
                                   uint64_t counter);

  void cache_public_key(const std::vector<uint8_t> &username,
                        const std::vector<uint8_t> &DH_pubkey,
                        const std::vector<uint8_t> &identity_key);
};
