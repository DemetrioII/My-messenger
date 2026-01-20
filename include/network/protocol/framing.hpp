#pragma once
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

class FramerMessage {

public:
	FramerMessage() = default;

	bool has_message_in_buffer(const std::vector<uint8_t>& buffer) const {
		if (buffer.size() < 4) return false;
		uint32_t len;
		std::memcpy(&len, buffer.data(), 4);
		return buffer.size() >= (4 + ntohl(len));
	}

	std::vector<uint8_t> extract_message(std::vector<uint8_t>& buffer) {
		uint32_t len;
		std::memcpy(&len, buffer.data(), 4);
		len = ntohl(len);
		std::vector<uint8_t> msg(buffer.begin() + 4, buffer.begin() + 4 + len);
		buffer.erase(buffer.begin(), buffer.begin() + 4 + len);
		return msg;
	}

	void form_message(const std::vector<uint8_t>& data, std::vector<uint8_t>& buffer) {
		uint32_t len = htonl(static_cast<uint32_t>(data.size()));
		buffer.insert(buffer.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
		buffer.insert(buffer.end(), data.begin(), data.end());
	}

	~FramerMessage() {

	}
};
