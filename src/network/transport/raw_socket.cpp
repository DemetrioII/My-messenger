#include "../../../include/network/transport/raw_socket.hpp"

int create_socket() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    std::cerr << "Error in creating socket" << std::endl;
    return -1;
  }

  return fd;
}

void TCPSocket::server(const std::string &ip, uint16_t port) {
  fd.reset_fd(create_socket());
  if (fd.get_fd() < 0)
    return;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (ip == "0.0.0.0")
    addr.sin_addr.s_addr = INADDR_ANY;
  else {
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
      fd.reset_fd(-1);
      throw std::runtime_error("Wrong IP: " + ip);
    }
  }

  if (::bind(fd.get_fd(), (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fd.reset_fd(-1);
    throw std::runtime_error("bind error");
  }

  if (::listen(fd.get_fd(), 10) < 0) {
    fd.reset_fd(-1);
    throw std::runtime_error("listen error");
  }

  int opt = 1;
  if (setsockopt(fd.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    fd.reset_fd(-1);
    throw std::runtime_error("setsockopt error: " +
                             std::string(strerror(errno)));
  }
}

int TCPSocket::client(const std::string &ip, uint16_t port) {
  fd.reset_fd(create_socket());
  if (fd.get_fd() < 0)
    return -1;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
    std::cerr << "inet_pton";
    return -EINVAL;
  }
  fcntl(fd.get_fd(), F_SETFL, fcntl(fd.get_fd(), F_GETFL, 0) | O_NONBLOCK);
  int res = ::connect(fd.get_fd(), (struct sockaddr *)&addr, sizeof(addr));
  if (res == 0) {
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

void UDPSocket::server(const std::string &ip, uint16_t port) {
  fd.reset_fd(create_socket());

  if (fd.get_fd() < 0)
    return;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (ip == "0.0.0.0")
    addr.sin_addr.s_addr = INADDR_ANY;
  else {
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
      fd.reset_fd(-1);
      throw std::runtime_error("Неверный IP: " + ip);
    }
  }
  if (bind(fd.get_fd(), (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    throw std::runtime_error("UDP bind failed");
  }
  int opt = 1;
  if (setsockopt(fd.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    fd.reset_fd(-1);
    throw std::runtime_error("setsockopt error: " +
                             std::string(strerror(errno)));
  }
}

int UDPSocket::client(const std::string &ip, uint16_t port) {
  fd.reset_fd(-1);
  if (fd.get_fd() < 0)
    return -1;

  memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
    std::cerr << ("inet_pton");
    return -1;
  }

  if (::connect(fd.get_fd(), (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    std::cerr << ("connect");
    return -2;
  }

  fcntl(fd.get_fd(), F_SETFL, fcntl(fd.get_fd(), F_GETFL, 0) | O_NONBLOCK);

  std::cout << "Подключено к серверу по адресу: " << ip << ":" << port << "\n";
  return 0;
}

bool UDPSocket::check_connection_complete(int timeout_ms) { return true; }

void ISocket::setup_server(const std::string &ip, uint16_t port) {
  switch (type) {
  case SocketType::TCP: {
    tcp = TCPSocket();
    tcp.server(ip, port);
    fd = tcp.fd.get_fd();
    break;
  }
  case SocketType::UDP: {
    udp = UDPSocket();
    udp.server(ip, port);
    fd = udp.fd.get_fd();
    break;
  }
  }
}

int ISocket::setup_connection(const std::string &ip, uint16_t port) {
  switch (type) {
  case SocketType::TCP: {
    tcp = TCPSocket();
    int r = tcp.client(ip, port);
    fd = tcp.fd.get_fd();
    return r;
  }
  case SocketType::UDP: {
    udp = UDPSocket();
    int r = udp.client(ip, port);
    fd = udp.fd.get_fd();
    return r;
  }
  }
}

bool ISocket::check_connection_complete(int timeout_ms) {
  switch (type) {
  case SocketType::TCP: {
    return tcp.check_connection_complete(timeout_ms);
  }
  case SocketType::UDP: {
    return udp.check_connection_complete(timeout_ms);
  }
  }
}
