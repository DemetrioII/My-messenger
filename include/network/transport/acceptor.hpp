//acceptor.hpp
#pragma once
#include <optional>
#include "interface.hpp"
#include "connection.hpp"
#include "transport.hpp"
#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <cstring>
#include <sys/epoll.h>

class Acceptor {
	TransportFabric transport_fabric;
public:
	Acceptor() = default;
	std::optional<std::shared_ptr<ClientConnection>> accept(int server_fd) {
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int client_fd = ::accept(server_fd, (struct sockaddr*)&addr, &addr_len);
		if (client_fd == -1) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				std::cerr << "Ошибка accept: " << strerror(errno) << std::endl;
			}
			return std::nullopt;
		}

		int flags = fcntl(client_fd, F_GETFL, 0);
		fcntl(client_fd, F_SETFL, O_NONBLOCK | flags);

		auto connection = std::make_shared<ClientConnection>(client_fd, addr);
		connection->init_transport(std::move(transport_fabric.create_tcp()));
		return connection;	
	}

	~Acceptor() = default;
};

