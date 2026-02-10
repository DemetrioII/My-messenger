#include "../../../include/network/transport/tls.hpp"
#include <iostream>

TLSWrapper::TLSWrapper(SSL_CTX *ctx, int fd) {
  client_fd_ = fd;
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, fd);

  if (SSL_accept(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
  } else {
    char buf[256];
    int n = SSL_read(ssl, buf, sizeof(buf) - 1);
    buf[n] = 0;

    printf("Received: %s\n", buf);

    SSL_write(ssl, "client hello", strlen("client hello"));
  }
}

TLSWrapper::~TLSWrapper() {
  SSL_shutdown(ssl);
  SSL_free(ssl);
}
