#include "../../include/encryption/identity_key.hpp"

IdentityKey::IdentityKey(void *pkey) : pkey_(pkey) {}
IdentityKey::~IdentityKey() {
  if (pkey_)
    EVP_PKEY_free(AS_PKEY(pkey_));
}

IdentityKey::IdentityKey(const IdentityKey &key) {
  if (key.pkey_) {
    EVP_PKEY_up_ref(AS_PKEY(key.pkey_));
    pkey_ = key.pkey_;
  }
}
IdentityKey &IdentityKey::operator=(const IdentityKey &other) {
  if (other.pkey_) {
    EVP_PKEY_up_ref(AS_PKEY(other.pkey_));
    pkey_ = other.pkey_;
  }
  return *this;
}

IdentityKey::IdentityKey(IdentityKey &&key) noexcept : pkey_(key.pkey_) {
  key.pkey_ = nullptr;
}

IdentityKey &IdentityKey::operator=(IdentityKey &&other) noexcept {
  if (this != &other) {
    if (pkey_) {
      EVP_PKEY_free(AS_PKEY(pkey_));
    }
    pkey_ = other.pkey_;
    other.pkey_ = nullptr;
  }
  return *this;
}

std::vector<uint8_t> IdentityKey::public_bytes() const {
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

std::vector<uint8_t> IdentityKey::private_bytes() const {
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
IdentityKey::compute_shared_secret(const IdentityKey &other_public_key) const {
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
