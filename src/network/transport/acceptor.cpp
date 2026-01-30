#include "../../../include/network/transport/acceptor.hpp"

std::optional<std::shared_ptr<ClientConnection>>
TCPAcceptor::accept(int server_fd) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  int client_fd = ::accept(server_fd, (struct sockaddr *)&addr, &addr_len);
  if (client_fd == -1) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      std::cerr << "Ошибка accept: " << strerror(errno) << std::endl;
    }
    return std::nullopt;
  }

  int flags = fcntl(client_fd, F_GETFL, 0);
  fcntl(client_fd, F_SETFL, O_NONBLOCK | flags);

  auto connection = std::make_shared<ClientConnection>(client_fd, addr);
  connection->init_transport(std::move(TransportFabric::create_tcp()));
  return connection;
}

TCPAcceptor::~TCPAcceptor() {}

std::optional<std::shared_ptr<ClientConnection>>
UDPAcceptor::accept(int server_fd) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  char buffer[1]; // dummy buffer to trigger recvfrom

  // Non-blocking recvfrom: just peek to see if a client sent anything
  ssize_t n =
      recvfrom(server_fd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT,
               (struct sockaddr *)&addr, &addr_len);
  if (n < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      std::cerr << "Ошибка recvfrom: " << strerror(errno) << std::endl;
    }
    return std::nullopt;
  }

  // If we peeked a packet, we consider this "client appeared"
  auto connection = std::make_shared<ClientConnection>(server_fd, addr);
  connection->init_transport(std::move(TransportFabric::create_udp(addr)));

  return connection;
}

UDPAcceptor::~UDPAcceptor() {}
