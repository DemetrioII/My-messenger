#pragma once
#include <algorithm>
#include <openssl/evp.h>
#include <string>
#include <vector>

class Base64 {
public:
  static std::string encode(const std::vector<uint8_t> &binary) {
    if (binary.empty())
      return "";
    // Рассчитываем размер (4 символа на каждые 3 байта + паддинг)
    int encoded_len = 4 * ((binary.size() + 2) / 3);
    std::vector<unsigned char> encoded(encoded_len +
                                       1); // +1 для null-terminator

    int len = EVP_EncodeBlock(encoded.data(), binary.data(), binary.size());
    return std::string(reinterpret_cast<char *>(encoded.data()), len);
  }

  static std::vector<uint8_t> decode(const std::string &base64_text) {
    if (base64_text.empty())
      return {};

    // Буфер для декодирования (максимально возможный размер)
    std::vector<unsigned char> decoded(base64_text.size());

    // OpenSSL EVP_DecodeBlock возвращает длину, включая паддинг (иногда больше
    // реальной)
    int len = EVP_DecodeBlock(
        decoded.data(),
        reinterpret_cast<const unsigned char *>(base64_text.data()),
        base64_text.size());

    if (len < 0)
      return {}; // Ошибка декодирования

    // Убираем паддинг (=) из конца, чтобы получить реальную длину
    // EVP_DecodeBlock туповат и может оставить нули в конце, если не учесть
    // паддинг вручную, но для AES GCM лишние нули в конце критичны (Tag не
    // сойдется). Простой хак для коррекции длины:
    int padding = 0;
    if (!base64_text.empty() && base64_text.back() == '=')
      padding++;
    if (base64_text.size() > 1 && base64_text[base64_text.size() - 2] == '=')
      padding++;

    decoded.resize(len - padding);

    // Конвертируем в uint8_t
    std::vector<uint8_t> result(decoded.begin(), decoded.end());
    return result;
  }
};
