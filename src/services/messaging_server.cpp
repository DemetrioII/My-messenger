#include "../../../include/services/messaging_server.hpp"

MessagingServer::MessagingServer()
    : server_context(std::make_shared<ServerContext>()) {
  dispatcher.registerHandler(MessageType::Text,
                             std::make_unique<TextMessageHandler>());

  dispatcher.registerHandler(MessageType::Command,
                             std::make_unique<MessageCommandHandler>());

  dispatcher.registerHandler(MessageType::CipherMessage,
                             std::make_unique<CipherMessageHandler>());

  dispatcher.registerHandler(MessageType::FileStart,
                             std::make_unique<FileStartHandler>());

  dispatcher.registerHandler(MessageType::FileChunk,
                             std::make_unique<FileChunkHandler>());

  dispatcher.registerHandler(MessageType::FileEnd,
                             std::make_unique<FileEndHandler>());

  /*server_context->messaging_service->user_logged_in.subscribe(
      [](const User &) { return true; },
      [this](const User &user) {
        int fd =
            server_context->messaging_service->get_fd_by_user_id(user.get_id());
        Message welcome_msg =
            server_context->parser.parse("Hello, " + user.get_id() + "!");
        server_context->transport_server->send(
            fd, server_context->serializer.serialize(welcome_msg));
      });

  server_context->messaging_service->message_sent.subscribe(
      [](auto &) { return true; },
      [this](auto &tpl) {
        auto [from_user_id, chat_id, msg] = tpl;
        auto &chat = server_context->messaging_service->get_chat_by_id(chat_id);
        for (auto &member : chat.get_members()) {
          int fd = server_context->messaging_service->get_fd_by_user_id(member);
          std::string prefix =
              "Message from " + from_user_id + " in chat " + chat_id + " ";
          std::vector<uint8_t> msg_bytes =
              std::vector<uint8_t>(prefix.begin(), prefix.end());
          msg_bytes.insert(msg_bytes.end(), msg.get_payload().begin(),
                           msg.get_payload().end());
          Message msg_to_send{msg_bytes, 0, {}, MessageType::Text};
          server_context->transport_server->send(
              fd, server_context->serializer.serialize(msg_to_send));
        }
      }); */
}

void MessagingServer::start_server(int port) {
  server_context->transport_server->start(port);
  server_context->transport_server->set_data_callback(
      [this](int fd, auto data) { on_tcp_data_received(fd, data); });
}

void MessagingServer::run() {
  server_context->transport_server->run_event_loop();
}

void MessagingServer::on_tcp_data_received(int fd,
                                           std::vector<uint8_t> &raw_data) {
  Message msg = server_context->serializer.deserialize(raw_data);

  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
}

void MessagingServer::send_error(int fd, const std::string &error_string) {
  server_context->transport_server->send(fd, {'e', 'r', 'r', 'o', 'r'});
}
