#pragma once
#include <iostream>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <stdexcept>

struct ServerTLSWrapper {
  int client_fd_;
  SSL *ssl;

  bool handshake_done = false;

  void tls_handshake();

  ServerTLSWrapper(SSL_CTX *ctx, int fd);
  ~ServerTLSWrapper();
};
