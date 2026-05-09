#include "include/infrastructure/crypto/identity_key.hpp"

IdentityKey::IdentityKey(void *pkey) : pkey_(pkey) {}

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

std::vector<uint8_t>
IdentityKey::sign(const std::vector<uint8_t> &message) const {
  if (!pkey_)
    throw std::runtime_error("No private key");

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    throw std::runtime_error("Failed to create MD Context");

  if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, AS_PKEY(pkey_)) <= 0) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("DigestSignInit failed");
  }

  size_t siglen = 0;
  if (EVP_DigestSign(ctx, nullptr, &siglen, message.data(), message.size()) <=
      0) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("DigestSign (size) failed");
  }

  std::vector<uint8_t> signature(siglen);
  if (EVP_DigestSign(ctx, signature.data(), &siglen, message.data(),
                     message.size()) <= 0) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("DigestSign failed");
  }

  EVP_MD_CTX_free(ctx);

  signature.resize(siglen);
  return signature;
}

bool IdentityKey::verify(const std::vector<uint8_t> &pubkey,
                         const std::vector<uint8_t> &signature) {
  if (!pkey_)
    throw std::runtime_error("No public key");

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    throw std::runtime_error("MD_CTX failed");

  if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, AS_PKEY(pkey_)) <=
      0) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("VerifyInit failed");
  }

  int rc = EVP_DigestVerify(ctx, signature.data(), signature.size(),
                            pubkey.data(), pubkey.size());

  EVP_MD_CTX_free(ctx);
  return rc == 1;
}

std::vector<uint8_t> IdentityKey::public_bytes() const {
  size_t len = 0;
  if (EVP_PKEY_get_raw_public_key(AS_PKEY(pkey_), nullptr, &len) <= 0) {
    throw std::runtime_error("Failed to get public key size");
  }

  std::vector<uint8_t> pub(len);

  if (EVP_PKEY_get_raw_public_key(AS_PKEY(pkey_), pub.data(), &len) <= 0) {
    throw std::runtime_error("Failed to get public key");
  }

  return pub;
}

std::vector<uint8_t> IdentityKey::private_bytes() const {
  size_t len = 0;

  if (EVP_PKEY_get_raw_private_key(AS_PKEY(pkey_), nullptr, &len) <= 0) {
    throw std::runtime_error("Failed to get private key size");
  }

  std::vector<uint8_t> priv(len);

  if (EVP_PKEY_get_raw_private_key(AS_PKEY(pkey_), priv.data(), &len) <= 0) {
    throw std::runtime_error("Failed to get private key");
  }

  return priv;
}

IdentityKey IdentityKey::from_private_bytes(const std::vector<uint8_t> &bytes) {
  EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr,
                                                bytes.data(), bytes.size());

  if (!pkey)
    throw std::runtime_error("Failed to create key from private bytes");

  return IdentityKey(pkey);
}

IdentityKey IdentityKey::from_public_bytes(const std::vector<uint8_t> &bytes) {
  EVP_PKEY *pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                               bytes.data(), bytes.size());

  if (!pkey)
    throw std::runtime_error("Failed to create from public bytes");

  return IdentityKey(pkey);
}

IdentityKey::~IdentityKey() {
  if (pkey_) {
    EVP_PKEY_free(AS_PKEY(pkey_));
    pkey_ = nullptr;
  }
}

DH_Key::DH_Key(void *pkey) : pkey_(pkey) {}
DH_Key::~DH_Key() {
  if (pkey_) {
    EVP_PKEY_free(AS_PKEY(pkey_));
    pkey_ = nullptr;
  }
}

DH_Key::DH_Key(const DH_Key &key) {
  if (key.pkey_) {
    EVP_PKEY_up_ref(AS_PKEY(key.pkey_));
    pkey_ = key.pkey_;
  }
}
DH_Key &DH_Key::operator=(const DH_Key &other) {
  if (other.pkey_) {
    EVP_PKEY_up_ref(AS_PKEY(other.pkey_));
    pkey_ = other.pkey_;
  }
  return *this;
}

DH_Key::DH_Key(DH_Key &&key) noexcept : pkey_(key.pkey_) {
  key.pkey_ = nullptr;
}

DH_Key &DH_Key::operator=(DH_Key &&other) noexcept {
  if (this != &other) {
    if (pkey_) {
      EVP_PKEY_free(AS_PKEY(pkey_));
    }
    pkey_ = other.pkey_;
    other.pkey_ = nullptr;
  }
  return *this;
}

std::vector<uint8_t> DH_Key::public_bytes() const {
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

std::vector<uint8_t> DH_Key::private_bytes() const {
  size_t len = 0;
  if (EVP_PKEY_get_raw_private_key(AS_PKEY(pkey_), NULL, &len) <= 0) {
    return {};
  }

  std::vector<uint8_t> buf(len);
  if (EVP_PKEY_get_raw_private_key(AS_PKEY(pkey_), buf.data(), &len) <= 0) {
    OPENSSL_cleanse(buf.data(), buf.size());
    return {};
  }

  return buf;
}

std::vector<uint8_t>
DH_Key::compute_shared_secret(const DH_Key &other_public_key) const {
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
    OPENSSL_cleanse(secret.data(), secret.size());
    secret.clear();
    EVP_PKEY_CTX_free(ctx);
    return {};
  }

  EVP_PKEY_CTX_free(ctx);
  return secret;
}
