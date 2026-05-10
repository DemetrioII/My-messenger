#include "include/application/commands/impl/private_message.hpp"

CommandType PrivateMessageCommand::getType() const {
  return CommandType::PRIVATE_MESSAGE;
}

void PrivateMessageCommand::fromParsedCommand(const ParsedCommand &pc) {
  if (pc.args.size() > 1) {
    recipient = pc.args[0];
    payload = pc.args[1];
  }
}

Message PrivateMessageCommand::toMessage() const {
  GetPubkeyCommand cmd;
  cmd.fromParsedCommand({.name = "getpub", .args{recipient}});
  return cmd.toMessage();
}

void PrivateMessageCommand::execeuteOnServer(
    std::shared_ptr<ServerContext> /*context*/) {}

void PrivateMessageCommand::executeOnClient(
    std::shared_ptr<ClientContext> context) {
  auto encryption_service = context->encryption_service;
  auto mq = context->mq;
  if (recipient.empty() || payload.empty()) {
    std::cout << "Usage: /pmess <recipient> <payload>" << std::endl;
    return;
  }
  GetPubkeyCommand cmd;
  ParsedCommand pc{.name = "getpub", .args{recipient}};
  cmd.fromParsedCommand(pc);
  cmd.executeOnClient(context);
  mq->push(recipient, payload);
  recipient.clear();
  payload.clear();
}

void PrivateMessageCommand::send_from_peer(
    std::shared_ptr<PeerContext> context) {
  if (recipient.empty() || payload.empty()) {
    std::cout << "Usage: /pmess <recipient> <payload>" << std::endl;
    return;
  }
  PeerApplicationService service;
  service.send_private_message(recipient, payload, context);
  std::cout << "Sent cipher message to "
            << std::string_view(
                   reinterpret_cast<const char *>(recipient.data()),
                   recipient.size())
            << std::endl;
}

void PrivateMessageCommand::recv_on_peer(int /*fd*/,
                                         std::shared_ptr<PeerContext> /*context*/) {
  std::cout << "Peer got a cipher message" << std::endl;
}

void PrivateMessageCommand::fromMessage(const Message &msg) {
  recipient = msg.get_meta(1);
  payload = msg.get_payload();
}

PrivateMessageCommand::~PrivateMessageCommand() {}
