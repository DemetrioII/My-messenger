#pragma once

#include <cstdint>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string>
#include <string_view>

namespace auth {

inline std::string generate_session_token(std::string_view user_pk_hex,
                                          uint64_t expires_at,
                                          const std::string &master_secret) {
  std::string payload =
      std::string(user_pk_hex) + ":" + std::to_string(expires_at);

  unsigned char hmac_result[EVP_MAX_MD_SIZE];
  unsigned int hmac_len = 0;

  unsigned char *res =
      HMAC(EVP_sha256(), master_secret.data(), master_secret.size(),
           reinterpret_cast<const unsigned char *>(payload.data()),
           payload.size(), hmac_result, &hmac_len);

  if (!res) {
    std::cerr << "Ошибка генерации HMAC!";
    return "";
  }

  std::string final_token =
      payload + "." +
      std::string(reinterpret_cast<char *>(hmac_result), hmac_len);

  return final_token;
}

inline bool verify_session_token(const std::string &token,
                                 const std::string &master_secret,
                                 uint64_t current_time) {
  // Разделяем токен на payload и подпись по точке
  size_t dot_pos = token.find('.');
  if (dot_pos == std::string::npos)
    return false;

  std::string payload = token.substr(0, dot_pos);
  std::string signature = token.substr(dot_pos + 1);

  // Парсим payload, чтобы проверить время жизни (expires_at)
  size_t colon_pos = payload.find(':');
  if (colon_pos == std::string::npos)
    return false;

  uint64_t expires_at = std::stoull(payload.substr(colon_pos + 1));
  if (current_time > expires_at) {
    std::cerr << "⚠️ Предъявлен протухший сессионный токен!";
    return false;
  }

  // Пересчитываем HMAC для проверки подлинности
  unsigned char expected_hmac[EVP_MAX_MD_SIZE];
  unsigned int hmac_len = 0;

  HMAC(EVP_sha256(), master_secret.data(), master_secret.size(),
       reinterpret_cast<const unsigned char *>(payload.data()), payload.size(),
       expected_hmac, &hmac_len);

  std::string expected_sig(reinterpret_cast<char *>(expected_hmac), hmac_len);

  // Криптографически безопасное сравнение строк (тайминг-атака мимо!)
  if (signature.size() == expected_sig.size() &&
      CRYPTO_memcmp(signature.data(), expected_sig.data(), signature.size()) ==
          0) {
    return true;
  }

  std::cerr << "🚨 Попытка подделки токена! Подпись не совпала.";
  return false;
}
} // namespace auth
