#include "../../../include/network/transport/tls.hpp"
#include <iostream>

void ServerTLSWrapper::tls_handshake() {
  int ret = SSL_accept(ssl);
  if (ret == 1) {
    handshake_done_ = 1;
    std::cout << "TLS handshake was done" << std::endl;
    return;
  }

  int err = SSL_get_error(ssl, ret);

  if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
    ERR_print_errors_fp(stderr);
    throw std::runtime_error("TLS SSL accept error");
    return;
  }
}

ServerTLSWrapper::ServerTLSWrapper(SSL_CTX *ctx, int fd) {
  client_fd_ = fd;
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, fd);

  tls_handshake();
}

ServerTLSWrapper::~ServerTLSWrapper() {
  if (ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }
}

void ClientTLSWrapper::tls_handshake() {
  int ret = SSL_connect(ssl);
  if (ret == 1) {
    handshake_done_ = 1;
    std::cout << "TLS handshake was done" << std::endl;
    return;
  }

  int err = SSL_get_error(ssl, ret);

  if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
    ERR_print_errors_fp(stderr);
    throw std::runtime_error("TLS SSL accept error");
    return;
  }
}

ClientTLSWrapper::ClientTLSWrapper(SSL_CTX *ctx, int fd) {
  server_fd_ = fd;
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, fd);

  tls_handshake();
}

ClientTLSWrapper::~ClientTLSWrapper() {
  if (ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }
}
