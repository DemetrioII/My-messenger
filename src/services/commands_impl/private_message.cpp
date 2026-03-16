#include "../../../include/services/commands_impl/private_message.hpp"

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
    std::shared_ptr<ServerContext> context) {}

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
    int fd, std::shared_ptr<PeerContext> context) {
  if (recipient.empty() || payload.empty()) {
    std::cout << "Usage: /pmess <recipient> <payload>" << std::endl;
    return;
  }
  auto res_fd = context->session_manager->get_fd(
      std::string(recipient.begin(), recipient.end()));
  if (!res_fd.has_value()) {
    switch (res_fd.error()) {
    case ServiceError::UserNotFound: {
      std::cout << "User not found" << std::endl;
      return;
    }
    default: {
      std::cout << "Unknown error" << std::endl;
      return;
    }
    }
  }
  auto cipher_message = context->encryption_service->encrypt_for(
      context->my_username, recipient, payload,
      context->messages_counter[recipient]);
  std::cout << "Sent cipher message to "
            << std::string_view(
                   reinterpret_cast<const char *>(recipient.data()),
                   recipient.size())
            << std::endl;
  send_to_peer(*context->peer_node, *res_fd,
               context->serializer->serialize(
                   {cipher_message,
                    2,
                    {context->my_username, context->encryption_service->sign()},
                    MessageType::CipherMessage}));
}

void PrivateMessageCommand::recv_on_peer(int fd,
                                         std::shared_ptr<PeerContext> context) {
  std::cout << "Peer got a cipher message" << std::endl;
}

void PrivateMessageCommand::fromMessage(const Message &msg) {
  recipient = msg.get_meta(1);
  payload = msg.get_payload();
}

PrivateMessageCommand::~PrivateMessageCommand() {}
