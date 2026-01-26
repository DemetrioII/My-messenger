#pragma once
#include "../bytes_hash.hpp"
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
  void set_identity_key();

  std::vector<uint8_t> get_public_bytes();

  std::vector<uint8_t> encrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &plaintext);

  std::vector<uint8_t> decrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &ciphertext);

  void cache_public_key(const std::vector<uint8_t> &username,
                        const std::vector<uint8_t> &pubkey);
};
