#pragma once
#include "interface.hpp"
#include <unistd.h>
#include <optional>
#include <cstring>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "../protocol/framing.hpp"

class ClientConnection : public IConnection {
    Fd fd;
	std::unique_ptr<FramerMessage> framer;
    struct sockaddr_in addr;
    std::unique_ptr<ITransport> transport;
    std::vector<uint8_t> recv_buffer; 
    std::vector<uint8_t> send_buffer; 
public:
    ClientConnection(int fd_) : fd(fd_) {}
    explicit ClientConnection(int fd_, const struct sockaddr_in& addr_) : fd(fd_), addr(addr_),
											framer(std::move(std::make_unique<FramerMessage>())) {}

    void init_transport(std::unique_ptr<ITransport> transport_) {
        transport = std::move(transport_);
    }

    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;

    // ЧИНЯЕМ MOVE: сбрасываем fd у донора
    ClientConnection(ClientConnection&& other) noexcept
        : fd(std::move(other.fd)), addr(other.addr), transport(std::move(other.transport)),
          recv_buffer(std::move(other.recv_buffer)), send_buffer(std::move(other.send_buffer)), framer(std::move(other.framer)) {
        other.fd = -1; 
    }

    ClientConnection& operator=(ClientConnection&& other) noexcept {
        if (this != &other) {
            fd = std::move(other.fd);
            addr = other.addr;
            transport = std::move(other.transport);
            recv_buffer = std::move(other.recv_buffer);
            send_buffer = std::move(other.send_buffer);
			framer = std::move(other.framer);
            other.fd = -1;
        }
        return *this;
    }

    bool flush() {
        if (send_buffer.empty()) return true;
        ssize_t sent = transport->send(fd.get_fd(), send_buffer);
        if (sent > 0) {
            send_buffer.erase(send_buffer.begin(), send_buffer.begin() + sent);
            return send_buffer.empty();
        }
        return (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK));
    }

    bool try_receive() {
        auto result = transport->receive(fd.get_fd());
        if (!result.data.empty()) {
            recv_buffer.insert(recv_buffer.end(), result.data.begin(), result.data.end());
        }
        return (result.status == ReceiveStatus::OK || result.status == ReceiveStatus::WOULDBLOCK);
    }

    // Проверяем, пришло ли сообщение целиком
    bool has_complete_message() const {
		return framer->has_message_in_buffer(recv_buffer);
	}

    std::vector<uint8_t> extract_message() {
        return framer->extract_message(recv_buffer);
    }

    void queue_send(const std::vector<uint8_t>& data) {
        framer->form_message(data, send_buffer);
    }

	auto get_addr() {
		return addr;
	}

    int get_fd() const { return fd.get_fd(); }
    ~ClientConnection() override {  }
};

 class ConnectionManager {
    std::mutex connections_mutex;
    std::unordered_map<int, std::shared_ptr<ClientConnection>> connections;
public:

    void add_connection(int fd, std::shared_ptr<ClientConnection> conn) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        connections[fd] = conn;
    }


    void remove_connection(int fd) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        if (find_connection(fd))
            connections.erase(fd);
    }


    bool find_connection(int fd) {
        return connections.find(fd) != connections.end();
    }


    void send_to_buffer(int fd, const std::vector<uint8_t>& data) {
        auto it = get_connection(fd);
        if (it != std::nullopt) {
            it->get()->queue_send(data);
            it->get()->flush();
        }
    }


    std::optional<std::shared_ptr<ClientConnection>> get_connection(int fd) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        if (find_connection(fd)) 
            return connections[fd];
        return std::nullopt;
    }


    ~ConnectionManager() {
        connections.clear();
    }
}; 
