#include "include/application/dispatching/message_dispatchers/server_file_handlers.hpp"

namespace {
std::string recipient_from(const Message &msg) {
  return std::string(msg.get_meta(0).begin(), msg.get_meta(0).end());
}
} // namespace

void ServerFileStartHandler::handleMessage(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  auto recipient_username = recipient_from(msg);
  auto recipient_fd_res = context->session_manager->get_fd(recipient_username);
  if (!recipient_fd_res.has_value()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::USER_NOT_FOUND);
    return;
  }
  context->transport_server->send(*recipient_fd_res,
                                  context->serializer.serialize(msg));
}

MessageType ServerFileStartHandler::getMessageType() const {
  return MessageType::FileStart;
}

void ServerFileChunkHandler::handleMessage(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  auto recipient_username = recipient_from(msg);
  auto recipient_fd_res = context->session_manager->get_fd(recipient_username);
  if (!recipient_fd_res.has_value()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::USER_NOT_FOUND);
    return;
  }
  context->transport_server->send(*recipient_fd_res,
                                  context->serializer.serialize(msg));
}

MessageType ServerFileChunkHandler::getMessageType() const {
  return MessageType::FileChunk;
}

void ServerFileEndHandler::handleMessage(
    const Message &msg, std::shared_ptr<ServerContext> context) {
  auto recipient_username = recipient_from(msg);
  auto recipient_fd_res = context->session_manager->get_fd(recipient_username);
  if (!recipient_fd_res.has_value()) {
    context->transport_server->send(context->fd,
                                    StaticResponses::USER_NOT_FOUND);
    return;
  }
  context->transport_server->send(*recipient_fd_res,
                                  context->serializer.serialize(msg));
}

MessageType ServerFileEndHandler::getMessageType() const {
  return MessageType::FileEnd;
}
