#include "interface.hpp"
#include "server.hpp"

class ServerBuilder {
private:
  std::unique_ptr<ISocket> socket_;
  std::unique_ptr<IAcceptor> acceptor_;
  std::unique_ptr<ITransport> transport_;
  bool use_tls = false;

public:
  ServerBuilder &set_socket(std::unique_ptr<ISocket> s) {
    socket_ = std::move(s);
    return *this;
  }

  ServerBuilder &set_acceptor(std::unique_ptr<IAcceptor> a) {
    acceptor_ = std::move(a);
    return *this;
  }

  ServerBuilder &enable_tls(bool enable) {
    use_tls = enable;
    return *this;
  }

  std::shared_ptr<Server> build() {
    if (!socket_)
      socket_ = std::make_unique<TCPSocket>();
    if (!acceptor_)
      acceptor_ = std::make_unique<TCPAcceptor>();

    auto server = Server::create(socket_, acceptor_);

    if (use_tls) {
      // ?
    }

    return server;
  }
};
