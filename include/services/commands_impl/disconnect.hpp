#pragma once
#include "../command_interface.hpp"

class DisconnectCommand : public ICommandHandler {
  std::string username;

public:
  CommandType getType() const override;

  void fromParsedCommand(const ParsedCommand &pc) override;

  Message toMessage() const override;

  void fromMessage(const Message &msg) override;

  void execeuteOnServer(std::shared_ptr<ServerContext> context) override;

  void executeOnClient(std::shared_ptr<ClientContext> context) override;

  void send_from_peer(std::shared_ptr<PeerContext> context) override;

  void recv_on_peer(int fd, std::shared_ptr<PeerContext> context) override;

  ~DisconnectCommand() override;
};
