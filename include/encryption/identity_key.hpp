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
  IdentityKey(void *pkey) : pkey_(pkey) {}
  ~IdentityKey() {
    if (pkey_)
      EVP_PKEY_free(AS_PKEY(pkey_));
  }

  IdentityKey(const IdentityKey &key) {
    if (key.pkey_) {
      EVP_PKEY_up_ref(AS_PKEY(key.pkey_));
      pkey_ = key.pkey_;
    }
  }
  IdentityKey &operator=(const IdentityKey &other) {
    if (other.pkey_) {
      EVP_PKEY_up_ref(AS_PKEY(other.pkey_));
      pkey_ = other.pkey_;
    }
    return *this;
  }

  IdentityKey(IdentityKey &&key) noexcept : pkey_(key.pkey_) {
    key.pkey_ = nullptr;
  }

  IdentityKey &operator=(IdentityKey &&other) noexcept {
    if (this != &other) {
      if (pkey_) {
        EVP_PKEY_free(AS_PKEY(pkey_));
      }
      pkey_ = other.pkey_;
      other.pkey_ = nullptr;
    }
    return *this;
  }
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

  std::vector<uint8_t> public_bytes() const {
    size_t len = 0;
    if (EVP_PKEY_get_raw_public_key(AS_PKEY(pkey_), NULL, &len) <= 0) {
      return {};
    }

    std::vector<uint8_t> buf(len);

    if (EVP_PKEY_get_raw_public_key(AS_PKEY(pkey_), buf.data(), &len) <= 0) {
      return {};
    }
    return buf;
  }

  std::vector<uint8_t> private_bytes() const {
    size_t len = 0;
    if (EVP_PKEY_get_raw_private_key(AS_PKEY(pkey_), NULL, &len) <= 0) {
      return {};
    }

    std::vector<uint8_t> buf(len);
    if (EVP_PKEY_get_raw_private_key(AS_PKEY(pkey_), buf.data(), &len) <= 0) {
      return {};
    }

    return buf;
  }

  std::vector<uint8_t>
  compute_shared_secret(const IdentityKey &other_public_key) const {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(AS_PKEY(pkey_), NULL);
    if (!ctx)
      return {};

    if (EVP_PKEY_derive_init(ctx) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      return {};
    }

    if (EVP_PKEY_derive_set_peer(ctx, AS_PKEY(other_public_key.pkey_)) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      return {};
    }

    size_t secret_len;
    EVP_PKEY_derive(ctx, NULL, &secret_len);

    std::vector<uint8_t> secret(secret_len);
    if (EVP_PKEY_derive(ctx, secret.data(), &secret_len) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      return {};
    }

    EVP_PKEY_CTX_free(ctx);
    return secret;
  }

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
