#include "../../include/encryption/AESGCMEncryption.hpp"

AESGCMEncryptor::AESGCMEncryptor() { ctx = EVP_CIPHER_CTX_new(); }

AESGCMEncryptor::~AESGCMEncryptor() { EVP_CIPHER_CTX_free(ctx); }

std::vector<uint8_t>
AESGCMEncryptor::encrypt(const std::vector<uint8_t> &key,
                         const std::vector<uint8_t> &plaintext,
                         uint64_t counter) {
  std::vector<uint8_t> iv(12);
  if (RAND_bytes(iv.data(), iv.size()) != 1) {
    throw std::runtime_error("Failed to generate IV");
  }
  EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data());

  std::vector<uint8_t> aad(sizeof(counter));
  for (size_t i = 0; i < sizeof(counter); ++i) {
    aad[i] = (counter >> (i * 8)) & 0xFF;
  }

  int aad_len;
  EVP_EncryptUpdate(ctx, nullptr, &aad_len, aad.data(), aad.size());

  std::vector<uint8_t> ciphertext(plaintext.size());
  int out_len1;
  EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len1, plaintext.data(),
                    plaintext.size());

  int out_len2;
  EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len1, &out_len2);

  ciphertext.resize(out_len1 + out_len2);

  std::vector<uint8_t> tag(16);
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data()) !=
      1)
    throw std::runtime_error("Failed to get GCM tag");

  std::vector<uint8_t> result;
  result.insert(result.end(), iv.begin(), iv.end());
  result.insert(result.end(), aad.begin(), aad.end());
  result.insert(result.end(), ciphertext.begin(), ciphertext.end());
  result.insert(result.end(), tag.begin(), tag.end());

  return result;
}

std::vector<uint8_t> AESGCMEncryptor::decrypt(const std::vector<uint8_t> &key,
                                              const std::vector<uint8_t> &input,
                                              uint64_t counter) {
  const size_t iv_len = 12;
  const size_t counter_len = 8;
  const size_t tag_len = 16;

  if (input.size() < iv_len + counter_len + tag_len) {
    throw std::runtime_error("Ciphertext too short");
  }

  const uint8_t *iv = input.data();
  const uint8_t *counter_bytes = input.data() + iv_len;
  const uint8_t *ciphertext = input.data() + iv_len + counter_len;
  size_t ciphertext_len = input.size() - iv_len - counter_len - tag_len;
  const uint8_t *tag = input.data() + iv_len + counter_len + ciphertext_len;

  uint64_t received_counter = 0;
  for (size_t i = 0; i < counter_len; ++i) {
    received_counter |= static_cast<uint64_t>(counter_bytes[i]) << (i * 8);
  }

  if (received_counter != counter) {
    throw std::runtime_error("Replay attack detected or counter mismatch");
  }

  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv) !=
      1) {
    throw std::runtime_error("EVP_DecryptInit_ex failed: " +
                             std::string(std::strerror(errno)));
  }

  std::vector<uint8_t> aad(counter_bytes, counter_bytes + counter_len);
  int aad_len;
  EVP_DecryptUpdate(ctx, nullptr, &aad_len, aad.data(), aad.size());

  std::vector<uint8_t> plaintext(ciphertext_len);
  int len1;
  if (EVP_DecryptUpdate(ctx, plaintext.data(), &len1, ciphertext,
                        ciphertext_len) != 1) {
    throw std::runtime_error("DecryptUpdate failed");
  }

  // установить GCM TAG перед финалом
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, (void *)tag) !=
      1) {
    throw std::runtime_error("SetTag failed");
  }

  int len2;
  if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len1, &len2) != 1) {
    throw std::runtime_error("DecryptFinal failed: authentication failure");
  }

  plaintext.resize(len1 + len2);
  return plaintext;
}
