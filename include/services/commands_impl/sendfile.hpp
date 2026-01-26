#pragma once
#include "../../models/message.hpp"
#include "../command_interface.hpp"
#include <fstream>
#include <thread>

class SendFileCommand : public ICommandHandler {
  std::string filename;
  std::vector<uint8_t> recipient;

  std::shared_ptr<IClient> client;

  Serializer serializer;

  static std::vector<uint8_t> to_bytes(uint64_t x) {
    std::vector<uint8_t> out(8);
    for (int i = 0; i < 8; i++)
      out[i] = (x >> (i * 8)) & 0xFF;
    return out;
  }

public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &parsed) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  bool isClientOnly() const override;

  ~SendFileCommand() override;
};
