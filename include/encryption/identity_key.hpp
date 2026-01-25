// identity_key.hpp
#pragma once
#include <openssl/err.h>
#include <openssl/evp.h>
#include <optional>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

#define AS_PKEY(ptr) static_cast<EVP_PKEY *>(ptr)

class IdentityKey {

public:
  IdentityKey() = default;
  IdentityKey(void *pkey);
  ~IdentityKey();

  IdentityKey(const IdentityKey &key);

  IdentityKey &operator=(const IdentityKey &other);

  IdentityKey(IdentityKey &&key) noexcept;

  IdentityKey &operator=(IdentityKey &&other) noexcept;

  static IdentityKey generate() {
    EVP_PKEY *pkey = nullptr;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);

    if (!ctx)
      throw std::runtime_error("Failed to create context");

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("Failed to generate X25519 key");
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("Failed to generate X25519 key");
    }

    EVP_PKEY_CTX_free(ctx);
    return IdentityKey(pkey);
  }

  std::vector<uint8_t> public_bytes() const;

  std::vector<uint8_t> private_bytes() const;

  std::vector<uint8_t>
  compute_shared_secret(const IdentityKey &other_public_key) const;

  static IdentityKey from_public_bytes(const std::vector<uint8_t> &pub_bytes) {
    EVP_PKEY *pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_X25519, nullptr, pub_bytes.data(), pub_bytes.size());
    if (!pkey)
      throw std::runtime_error(
          "Failed to create IdentityKey from public bytes");
    return IdentityKey(pkey);
  }

  static IdentityKey from_private_bytes(const std::vector<uint8_t> &);

private:
  void *pkey_ = nullptr;
};
