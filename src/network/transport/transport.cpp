#include "../../../include/network/transport/transport.hpp"
#include <iostream>

std::optional<std::vector<uint8_t>>
extract_complete_message(std::vector<uint8_t> &buffer) {
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

ssize_t TCPTransport::send(const std::vector<uint8_t> &data) {
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

ReceiveResult TCPTransport::receive() {
  char temp[4096]; // Больше для эффективности
  std::vector<uint8_t> accumulation_buffer;

  while (true) {
    ssize_t bytes = ::recv(fd, temp, sizeof(temp), 0);

    if (bytes > 0) {
      accumulation_buffer.insert(accumulation_buffer.end(), temp, temp + bytes);

      auto message = extract_complete_message(accumulation_buffer);
      if (message.has_value()) {
        ReceiveResult res;
        res.status = ReceiveStatus::OK;
        res.data.set_data(message->data(), message->size());
        res.error_code = 0;

        return res;
      }
      // Иначе продолжаем читать (в буфере может быть больше данных)
    } else if (bytes == 0) {
      // Соединение закрыто
      ReceiveResult res;
      res.status = ReceiveStatus::CLOSED;
      res.data.set_data(accumulation_buffer.data(), accumulation_buffer.size());
      res.error_code = 0;
      return res; // Возвращаем то, что успели прочитать
    } else {      // bytes == -1
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Нет доступных данных прямо сейчас
        ReceiveResult res;
        res.status = ReceiveStatus::WOULDBLOCK;
        res.data.set_data(accumulation_buffer.data(),
                          accumulation_buffer.size());
        res.error_code = 0;
        return res; // Возвращаем уже прочитанное
      } else {
        // Ошибка
        ReceiveResult res;
        res.status = ReceiveStatus::ERROR;
        res.data.set_data(accumulation_buffer.data(),
                          accumulation_buffer.size());
        res.error_code = errno;
        return res;
        // Возвращаем то, что успели
      }
    }
  }
}

TCPTransport::~TCPTransport() {}

ssize_t UDPTransport::send(const std::vector<uint8_t> &data) {
  try {
    if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
      return 0;
    }
    std::vector<uint8_t> send_buffer;
    uint32_t len = htonl(data.size());
    const uint8_t *p = reinterpret_cast<uint8_t *>(&len);

    send_buffer.insert(send_buffer.end(), p, p + sizeof(uint32_t));
    send_buffer.insert(send_buffer.end(), data.begin(), data.end());
    ssize_t sent = ::sendto(fd, send_buffer.data(), send_buffer.size(), 0,
                            (const sockaddr *)&addr, sizeof(addr));
    return (sent > 0) ? (sent - sizeof(uint32_t)) : sent;

  } catch (std::exception e) {
    return 0;
  }
}

ReceiveResult UDPTransport::receive() {
  char temp[4096]; // Больше для эффективности
  std::vector<uint8_t> buffer;

  sockaddr_in from_addr;
  socklen_t addr_len = sizeof(from_addr);
  ssize_t bytes =
      ::recvfrom(fd, temp, sizeof(temp), 0, (sockaddr *)&from_addr, &addr_len);

  if (bytes > 0) {
    if (from_addr.sin_addr.s_addr != addr.sin_addr.s_addr ||
        from_addr.sin_port != addr.sin_port) {
      return {ReceiveStatus::ERROR, {}, EINVAL};
    }
    if (bytes < static_cast<ssize_t>(sizeof(uint32_t))) {
      return {ReceiveStatus::ERROR, {}, EBADMSG};
    }

    uint32_t declared_len;
    ::memcpy(&declared_len, temp, sizeof(uint32_t));
    declared_len = ntohl(declared_len);

    if (bytes != static_cast<ssize_t>(sizeof(uint32_t) + declared_len)) {
      return {ReceiveStatus::ERROR, {}, EBADMSG};
    }

    std::vector<uint8_t> message(temp + sizeof(uint32_t), temp + bytes);
    ReceiveResult res;
    res.status = ReceiveStatus::OK;
    res.data.set_data(
        reinterpret_cast<const uint8_t *>(temp + sizeof(uint32_t)),
        declared_len);
    res.error_code = 0;
    return res;
  } else if (bytes == 0) {
    // Соединение закрыто
    ReceiveResult res;
    res.status = ReceiveStatus::CLOSED;
    res.data = {};
    res.error_code = 0;
    return res;
    // Возвращаем то, что успели прочитать
  } else { // bytes == -1
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Нет доступных данных прямо сейчас
      ReceiveResult res;
      res.status = ReceiveStatus::WOULDBLOCK;
      res.data = {};
      res.error_code = 0;
      return res;
      // Возвращаем уже прочитанное
    } else {
      // Ошибка
      ReceiveResult res;
      res.status = ReceiveStatus::ERROR;
      res.data = {};
      res.error_code = errno;
      return res; // Возвращаем то, что успели
    }
  }
}

UDPTransport::~UDPTransport() {}

ssize_t TLSTransport::send(const std::vector<uint8_t> &data) {
  try {
    std::vector<uint8_t> send_buffer;
    uint32_t len = htonl(data.size());
    const uint8_t *p = reinterpret_cast<uint8_t *>(&len);

    send_buffer.insert(send_buffer.end(), p, p + sizeof(uint32_t));
    send_buffer.insert(send_buffer.end(), data.begin(), data.end());
    return SSL_write(ssl_, send_buffer.data(), send_buffer.size()) -
           sizeof(uint32_t);
  } catch (std::exception e) {
    return 0;
  }
}

ReceiveResult TLSTransport::receive() {
  char temp[4096]; // Больше для эффективности
  std::vector<uint8_t> accumulation_buffer;

  while (true) {
    ssize_t bytes = SSL_read(ssl_, temp, sizeof(temp));
    std::cout << "TLS transport received " << bytes << " bytes" << std::endl;

    if (bytes > 0) {
      accumulation_buffer.insert(accumulation_buffer.end(), temp, temp + bytes);

      auto message = extract_complete_message(accumulation_buffer);
      if (message.has_value()) {
        ReceiveResult res;
        res.status = ReceiveStatus::OK;
        res.data.set_data(message->data(), message->size());
        res.error_code = 0;

        return res;
      }
      // Иначе продолжаем читать (в буфере может быть больше данных)
    } else if (bytes == 0) {
      // Соединение закрыто
      ReceiveResult res;
      res.status = ReceiveStatus::CLOSED;
      res.data.set_data(accumulation_buffer.data(), accumulation_buffer.size());
      res.error_code = 0;
      return res; // Возвращаем то, что успели прочитать
    } else {      // bytes == -1
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Нет доступных данных прямо сейчас
        ReceiveResult res;
        res.status = ReceiveStatus::WOULDBLOCK;
        res.data.set_data(accumulation_buffer.data(),
                          accumulation_buffer.size());
        res.error_code = 0;
        return res; // Возвращаем уже прочитанное
      } else {
        // Ошибка
        ReceiveResult res;
        res.status = ReceiveStatus::ERROR;
        res.data.set_data(accumulation_buffer.data(),
                          accumulation_buffer.size());
        res.error_code = errno;
        return res;
        // Возвращаем то, что успели
      }
    }
  }
}

TLSTransport::~TLSTransport() {}
