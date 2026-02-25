#pragma once
#include "../bytes_hash.hpp"
#include "../encryption/AESGCMEncryption.hpp"
#include "../encryption/KDF.hpp"
#include "../encryption/identity_key.hpp"
#include "../models/message.hpp"

class EncryptionService {
  DH_Key DH_key;
  IdentityKey identity_key;

  AESGCMEncryptor aes_gcm_encryptor;

  std::unordered_map<std::vector<uint8_t>, DH_Key> keys;

  auto load_or_generate_key() { return DH_Key::generate(); }

public:
  void set_identity_key();

  std::vector<uint8_t> get_public_bytes();

  std::vector<uint8_t> encrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &plaintext,
                                   uint64_t counter);

  std::vector<uint8_t> decrypt_for(const std::vector<uint8_t> &sender,
                                   const std::vector<uint8_t> &username,
                                   const std::vector<uint8_t> &ciphertext,
                                   uint64_t counter);

  void cache_public_key(const std::vector<uint8_t> &username,
                        const std::vector<uint8_t> &pubkey);
};
