// identity_key.hpp
#pragma once
#include <fstream>
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

  static bool file_exists(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
  }

  static std::vector<uint8_t> load_file(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
      throw std::runtime_error("Failed to open file");

    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),
                                std::istreambuf_iterator<char>());
  }

  static void save_file(const std::string &path,
                        const std::vector<uint8_t> &data) {
    std::ofstream f(path, std::ios::binary);
    if (!f)
      throw std::runtime_error("Failed to write file");

    f.write(reinterpret_cast<const char *>(data.data()), data.size());
  }

  static IdentityKey generate() {
    EVP_PKEY *pkey = nullptr;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);

    if (!ctx)
      throw std::runtime_error("Failed to create context");

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("Failed to generate Ed25519 key");
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("Failed to generate Ed25519 key");
    }

    EVP_PKEY_CTX_free(ctx);
    return IdentityKey(pkey);
  }

  std::vector<uint8_t> sign(const std::vector<uint8_t> &message) const;

  bool verify(const std::vector<uint8_t> &pubkey,
              const std::vector<uint8_t> &signature);

  std::vector<uint8_t> public_bytes() const;

  std::vector<uint8_t> private_bytes() const;

  static IdentityKey from_private_bytes(const std::vector<uint8_t> &);

  static IdentityKey from_public_bytes(const std::vector<uint8_t> &);

private:
  void *pkey_;
};

class DH_Key {

public:
  DH_Key() = default;
  DH_Key(void *pkey);
  ~DH_Key();

  DH_Key(const DH_Key &key);

  DH_Key &operator=(const DH_Key &other);

  DH_Key(DH_Key &&key) noexcept;

  DH_Key &operator=(DH_Key &&other) noexcept;

  static DH_Key generate() {
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
    return DH_Key(pkey);
  }

  std::vector<uint8_t> public_bytes() const;

  std::vector<uint8_t> private_bytes() const;

  std::vector<uint8_t>
  compute_shared_secret(const DH_Key &other_public_key) const;

  static DH_Key from_public_bytes(const std::vector<uint8_t> &pub_bytes) {
    EVP_PKEY *pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_X25519, nullptr, pub_bytes.data(), pub_bytes.size());
    if (!pkey)
      throw std::runtime_error(
          "Failed to create IdentityKey from public bytes");
    return DH_Key(pkey);
  }

  static DH_Key from_private_bytes(const std::vector<uint8_t> &);

private:
  void *pkey_ = nullptr;
};
