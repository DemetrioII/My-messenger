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
    return -EINVAL;
  }

  fcntl(fd.get_fd(), F_SETFL, fcntl(fd.get_fd(), F_GETFL, 0) | O_NONBLOCK);

  int result =
      ::connect(fd.get_fd(), (struct sockaddr *)&address, sizeof(address));

  if (result == 0) {
    std::cerr << "Immediate connection to " << ip << ":" << port << std::endl;
    return 0;
  }

  switch (errno) {
  case EINPROGRESS:
    std::cout << "Connection in progress to " << ip << ":" << port
              << " (EINPROGRESS)" << std::endl;
    return 0;
  case EISCONN:
    std::cout << "Already connected to " << ip << ":" << port << std::endl;
    return 0;
  case EALREADY:
    std::cout << "Connection already in progress to " << ip << ":" << port
              << std::endl;
    return 0;
  case ECONNREFUSED:
    std::cout << "Connection refused (normal for P2P): " << ip << ":" << port
              << std::endl;
    return -ECONNREFUSED;
  case ETIMEDOUT:
    std::cerr << "Connection timeout to " << ip << ":" << port << std::endl;
    return -ETIMEDOUT;
  default:
    std::cerr << "Connect error to " << ip << ":" << port << ": "
              << strerror(errno) << std::endl;
    return -errno;
  }

  std::cout << "Подключено к серверу по адресу: " << ip << ":" << port << "\n";
  return 0;
}

sockaddr_in TCPSocket::get_peer_address() const { return address; }

bool TCPSocket::check_connection_complete(int timeout_ms) {
  fd_set writefds, exceptfds;
  struct timeval tv;

  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);
  FD_SET(fd.get_fd(), &writefds);
  FD_SET(fd.get_fd(), &exceptfds);

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  int ret = select(fd.get_fd() + 1, NULL, &writefds, &exceptfds, &tv);

  if (ret > 0) {
    if (FD_ISSET(fd.get_fd(), &exceptfds)) {
      // Ошибка соединения
      int error = 0;
      socklen_t len = sizeof(error);
      getsockopt(fd.get_fd(), SOL_SOCKET, SO_ERROR, &error, &len);
      std::cerr << "Connection failed: " << strerror(error) << std::endl;
      return false;
    }

    if (FD_ISSET(fd.get_fd(), &writefds)) {
      // Проверяем успешность
      int error = 0;
      socklen_t len = sizeof(error);
      getsockopt(fd.get_fd(), SOL_SOCKET, SO_ERROR, &error, &len);
      return (error == 0);
    }
  }

  return false; // Таймаут или ошибка select
}

// НОВЫЙ МЕТОД: получить ошибку сокета (если есть)
int TCPSocket::get_socket_error() const {
  int error = 0;
  socklen_t len = sizeof(error);
  if (getsockopt(fd.get_fd(), SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
    return errno;
  }
  return error;
}

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

bool UDPSocket::check_connection_complete(int timeout_ms) { return true; }

int UDPSocket::get_socket_error() const { return 0; }

sockaddr_in UDPSocket::get_peer_address() const { return address; }

SocketType UDPSocket::get_type() const { return SocketType::UDP; }

int UDPSocket::get_fd() const { return fd.get_fd(); }

void UDPSocket::close() { fd.reset_fd(-1); }

UDPSocket::~UDPSocket() {}
