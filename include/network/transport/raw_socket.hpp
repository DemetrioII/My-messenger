// raw_socket.hpp
#pragma once
#include "interface.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 10

class TCPSocket : public ISocket {
  Fd fd;

  struct sockaddr_in address;

public:
  int create_socket() override {
    fd.reset_fd(socket(AF_INET, SOCK_STREAM, 0));

    if (fd.get_fd() < 0) {
      std::cout << ("Ошибка создания сокета") << std::endl;
      return -1;
    }

    return fd.get_fd();
  }

  void make_address_reusable() override {
    int opt = 1;
    if (setsockopt(fd.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
      close();
      throw std::runtime_error("Ошибка setsockopt: " +
                               std::string(strerror(errno)));
    }
  }

  void setup_server(uint16_t PORT, const std::string &ip) override {
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

  int setup_connection(const std::string &ip, uint16_t port) override {
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

    std::cout << "Подключено к серверу по адресу: " << ip << ":" << port
              << "\n";
    return 0;
  }

  sockaddr_in get_peer_address() const override { return address; }

  int get_fd() const override { return fd.get_fd(); }

  void close() override { fd.reset_fd(-1); }

  SocketType get_type() const override { return SocketType::TCP; }

  ~TCPSocket() override {}
};

class UDPSocket : public ISocket {
  Fd fd;

  struct sockaddr_in address;

public:
  int create_socket() override {
    fd.reset_fd(socket(AF_INET, SOCK_DGRAM, 0));

    if (fd.get_fd() < 0) {
      std::cerr << "[Error]: Socket creating" << std::endl;
      return -1;
    }
    return fd.get_fd();
  }

  void setup_server(uint16_t port, const std::string &ip) override {
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

  void make_address_reusable() override {
    int opt = 1;
    if (setsockopt(fd.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
      close();
      throw std::runtime_error("Ошибка setsockopt: " +
                               std::string(strerror(errno)));
    }
  }

  int setup_connection(const std::string &ip, uint16_t port) override {
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

    std::cout << "Подключено к серверу по адресу: " << ip << ":" << port
              << "\n";
    return 0;
  }

  sockaddr_in get_peer_address() const override { return address; }

  SocketType get_type() const override { return SocketType::UDP; }

  int get_fd() const override { return fd.get_fd(); }

  void close() override { fd.reset_fd(-1); }

  ~UDPSocket() override {}
};
