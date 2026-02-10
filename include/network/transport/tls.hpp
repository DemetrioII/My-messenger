#pragma once
#include <iostream>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <stdexcept>

struct TLSWrapper {
  int client_fd_;
  SSL *ssl;

  TLSWrapper(SSL_CTX *ctx, int fd);
  ~TLSWrapper();
};
