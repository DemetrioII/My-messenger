// transport.hpp
#pragma once
#include "interface.hpp"
#include <memory.h>
#include <optional>

class TCPTransport : public ITransport {
  std::optional<std::vector<uint8_t>>
  extract_complete_message(int fd, std::vector<uint8_t> &buffer) const {
    if (buffer.size() < sizeof(uint32_t)) {
      return std::nullopt;
    }

    uint32_t len;
    ::memcpy(&len, buffer.data(), sizeof(uint32_t));
    len = ntohl(len);
    if (buffer.size() < sizeof(uint32_t) + len) {
      return std::nullopt;
    }

    std::vector<uint8_t> message(buffer.begin() + sizeof(uint32_t),
                                 buffer.begin() + sizeof(uint32_t) + len);

    buffer.erase(buffer.begin(), buffer.begin() + sizeof(uint32_t) + len);
    return message;
  }

public:
  TCPTransport() = default;

  ssize_t send(int fd, const std::vector<uint8_t> &data) const override {
    try {
      std::vector<uint8_t> send_buffer;
      uint32_t len = htonl(data.size());
      const uint8_t *p = reinterpret_cast<uint8_t *>(&len);

      send_buffer.insert(send_buffer.end(), p, p + sizeof(uint32_t));
      send_buffer.insert(send_buffer.end(), data.begin(), data.end());
      return ::send(fd, send_buffer.data(), send_buffer.size(), 0) -
             sizeof(uint32_t);

    } catch (std::exception e) {
      return 0;
    }
  }

  ReceiveResult receive(int fd) const override {
    char temp[4096]; // Больше для эффективности
    std::vector<uint8_t> buffer;

    while (true) {
      ssize_t bytes = ::recv(fd, temp, sizeof(temp), 0);

      if (bytes > 0) {
        buffer.insert(buffer.end(), temp, temp + bytes);

        auto message = extract_complete_message(fd, buffer);
        if (message.has_value()) {
          // Возвращаем то, что уже прочитали
          return {ReceiveStatus::OK, message.value(), 0};
        }
        // Иначе продолжаем читать (в буфере может быть больше данных)
      } else if (bytes == 0) {
        // Соединение закрыто
        return {ReceiveStatus::CLOSED, buffer,
                0}; // Возвращаем то, что успели прочитать
      } else { // bytes == -1
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // Нет доступных данных прямо сейчас
          return {ReceiveStatus::WOULDBLOCK, buffer,
                  0}; // Возвращаем уже прочитанное
        } else {
          // Ошибка
          return {ReceiveStatus::ERROR, buffer,
                  errno}; // Возвращаем то, что успели
        }
      }
    }
  }

  ~TCPTransport() override {}
};

class TransportFabric {
public:
  std::unique_ptr<TCPTransport> create_tcp() {
    return std::make_unique<TCPTransport>(TCPTransport());
  }

  /* std::unique_ptr<UDPTransport> create_udp() {
    return std::make_unique<UDPTransport>(UDPTransport());
  } */
};
