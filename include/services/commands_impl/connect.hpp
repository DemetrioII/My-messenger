#pragma once
#include "../command_interface.hpp"

class ConnectCommand : public ICommandHandler {
  std::string ip;
  int port;
  std::vector<uint8_t> DH_bytes;
  std::vector<uint8_t> identity_bytes;
  std::vector<uint8_t> signature;
  std::vector<uint8_t> username;

public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &pc) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  void send_from_peer(int fd, std::shared_ptr<PeerContext> context) override;

  void recv_on_peer(int fd, std::shared_ptr<PeerContext> context) override;

  ~ConnectCommand() override;
};
