#pragma once
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <stdexcept>
#include <string>
#include <vector>

class HKDF {
public:
  static std::vector<uint8_t> derive(const std::vector<uint8_t> &ikm,
                                     const std::vector<uint8_t> &salt = {},
                                     const std::vector<uint8_t> &info = {},
                                     size_t output_len = 32) {

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (!ctx) {
      throw std::runtime_error("Failed to create HKDF context");
    }

    if (EVP_PKEY_derive_init(ctx) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("HKDF init failed");
    }

    if (EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256()) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("Failed to set HKDF hash");
    }

    if (EVP_PKEY_CTX_set1_hkdf_key(ctx, ikm.data(), ikm.size()) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("Failed to set HKDF key");
    }

    if (!salt.empty()) {
      if (EVP_PKEY_CTX_set1_hkdf_salt(ctx, salt.data(), salt.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to set HKDF salt");
      }
    }

    if (!info.empty()) {
      if (EVP_PKEY_CTX_add1_hkdf_info(ctx, info.data(), info.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to set HKDF info");
      }
    }

    std::vector<uint8_t> output_key(output_len);
    size_t out_len = output_len;

    if (EVP_PKEY_derive(ctx, output_key.data(), &out_len) <= 0) {
      EVP_PKEY_CTX_free(ctx);
      throw std::runtime_error("HKDF derivation failed");
    }

    EVP_PKEY_CTX_free(ctx);
    return output_key;
  }

  static std::vector<uint8_t>
  derive_for_messaging(const std::vector<uint8_t> &shared_secret,
                       const std::vector<uint8_t> &sender_id,
                       const std::vector<uint8_t> &receiver_id,
                       const std::string &purpose = "encryption") {
    std::vector<uint8_t> salt;
    if (sender_id < receiver_id) {
      salt.insert(salt.end(), sender_id.begin(), sender_id.end());
      salt.insert(salt.end(), receiver_id.begin(), receiver_id.end());
    } else {
      salt.insert(salt.end(), receiver_id.begin(), receiver_id.end());
      salt.insert(salt.end(), sender_id.begin(), sender_id.end());
    }

    std::string info_str = "messaging-key-" + purpose;
    std::vector<uint8_t> info(info_str.begin(), info_str.end());

    return derive(shared_secret, salt, info, 32);
  }
};
