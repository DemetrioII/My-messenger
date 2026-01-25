#pragma once
#include <cerrno>
#include <cstring>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <vector>

class AESGCMEncryptor {
  EVP_CIPHER_CTX *ctx;

public:
  AESGCMEncryptor();

  ~AESGCMEncryptor();

  std::vector<uint8_t> encrypt(const std::vector<uint8_t> &key,
                               const std::vector<uint8_t> &plaintext);

  std::vector<uint8_t> decrypt(const std::vector<uint8_t> &key,
                               const std::vector<uint8_t> &input);
};
