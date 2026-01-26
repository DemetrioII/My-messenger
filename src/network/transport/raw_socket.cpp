#include "../../../include/network/transport/raw_socket.hpp"

int TCPSocket::create_socket() {
  fd.reset_fd(socket(AF_INET, SOCK_STREAM, 0));

  if (fd.get_fd() < 0) {
    std::cout << ("Ошибка создания сокета") << std::endl;
    return -1;
  }

  return fd.get_fd();
}

void TCPSocket::make_address_reusable() {
  int opt = 1;
  if (setsockopt(fd.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    close();
    throw std::runtime_error("Ошибка setsockopt: " +
                             std::string(strerror(errno)));
  }
}

void TCPSocket::setup_server(uint16_t PORT, const std::string &ip) {
  address.sin_family = AF_INET;
  address.sin_port = htons(PORT);
  if (ip == "0.0.0.0")
    address.sin_addr.s_addr = INADDR_ANY;
  else {
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
      close();
      throw std::runtime_error("Неверный IP: " + ip);
    }
  }

  if (bind(fd.get_fd(), (struct sockaddr *)&address, sizeof(address)) < 0) {
    close();
    throw std::runtime_error("Ошибка bind");
  }

  if (listen(fd.get_fd(), 10) < 0) {
    close();
    throw std::runtime_error("Ошибка listen");
  }
}

int TCPSocket::setup_connection(const std::string &ip, uint16_t port) {
  memset(&address, 0, sizeof(address));

  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
    std::cerr << ("inet_pton");
    return -1;
  }

  if (::connect(fd.get_fd(), (struct sockaddr *)&address, sizeof(address)) <
      0) {
    std::cerr << ("connect");
    return -2;
  }

  fcntl(fd.get_fd(), F_SETFL, fcntl(fd.get_fd(), F_GETFL, 0) | O_NONBLOCK);

  std::cout << "Подключено к серверу по адресу: " << ip << ":" << port << "\n";
  return 0;
}

sockaddr_in TCPSocket::get_peer_address() const { return address; }

int TCPSocket::get_fd() const { return fd.get_fd(); }

void TCPSocket::close() { fd.reset_fd(-1); }

SocketType TCPSocket::get_type() const { return SocketType::TCP; }

TCPSocket::~TCPSocket() {}

int UDPSocket::create_socket() {
  fd.reset_fd(socket(AF_INET, SOCK_DGRAM, 0));

  if (fd.get_fd() < 0) {
    std::cerr << "[Error]: Socket creating" << std::endl;
    return -1;
  }
  return fd.get_fd();
}

void UDPSocket::setup_server(uint16_t port, const std::string &ip) {
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (ip == "0.0.0.0")
    address.sin_addr.s_addr = INADDR_ANY;
  else {
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
      close();
      throw std::runtime_error("Неверный IP: " + ip);
    }
  }
  if (bind(fd.get_fd(), (struct sockaddr *)&address, sizeof(address)) < 0) {
    throw std::runtime_error("UDP bind failed");
  }
}

void UDPSocket::make_address_reusable() {
  int opt = 1;
  if (setsockopt(fd.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    close();
    throw std::runtime_error("Ошибка setsockopt: " +
                             std::string(strerror(errno)));
  }
}

int UDPSocket::setup_connection(const std::string &ip, uint16_t port) {
  memset(&address, 0, sizeof(address));

  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
    std::cerr << ("inet_pton");
    return -1;
  }

  if (::connect(fd.get_fd(), (struct sockaddr *)&address, sizeof(address)) <
      0) {
    std::cerr << ("connect");
    return -2;
  }

  fcntl(fd.get_fd(), F_SETFL, fcntl(fd.get_fd(), F_GETFL, 0) | O_NONBLOCK);

  std::cout << "Подключено к серверу по адресу: " << ip << ":" << port << "\n";
  return 0;
}

sockaddr_in UDPSocket::get_peer_address() const { return address; }

SocketType UDPSocket::get_type() const { return SocketType::UDP; }

int UDPSocket::get_fd() const { return fd.get_fd(); }

void UDPSocket::close() { fd.reset_fd(-1); }

UDPSocket::~UDPSocket() {}
