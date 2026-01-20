//raw_socket.hpp
#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <cstring>
#include <sys/epoll.h>
#include "interface.hpp"

#define MAX_EVENTS 10


class TCPSocket : public ISocket {
	Fd fd;
public:
	int create_socket() override {
		fd.reset_fd(socket(AF_INET, SOCK_STREAM, 0));

		if (fd.get_fd() < 0) {
			std::cout << ("Ошибка создания сокета") << std::endl;
			return -1;
		}

		return fd.get_fd();
	}

	void make_address_reusable() {
		int opt = 1;
		if (setsockopt(fd.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			close();
			throw std::runtime_error("Ошибка setsockopt: " + std::string(strerror(errno)));
		}
	}

	void setup_server(uint16_t PORT, const std::string& ip) override {
		struct sockaddr_in address;
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

		if (bind(fd.get_fd(), (struct sockaddr*)&address, sizeof(address)) < 0) {
			close();
			throw std::runtime_error("Ошибка bind");
		}

		if (listen(fd.get_fd(), 10) < 0) {
			close();
			throw std::runtime_error("Ошибка listen");
		}
	}

	
	int setup_connection(const std::string& ip, uint16_t port) {
		struct sockaddr_in sockaddr;
		memset(&sockaddr, 0, sizeof(sockaddr));

		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = htons(port);

		if (inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr) <= 0) {
			std:: cerr << ("inet_pton");
			return -1;
		}

		if (::connect(fd.get_fd(), (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
			std::cerr << ("connect");
			return -2;
		}

		fcntl(fd.get_fd(), F_SETFL, fcntl(fd.get_fd(), F_GETFL, 0) | O_NONBLOCK);

		std::cout << "Подключено к серверу по адресу: " << ip << ":" << port << "\n";
		return 0;
	}


	int get_fd() const override {
		return fd.get_fd();
	}

	void close() override {
		fd.reset_fd(-1);
	}

	~TCPSocket() override {
	
	}
};

