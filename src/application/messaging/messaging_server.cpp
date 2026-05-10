#include "include/application/messaging/messaging_server.hpp"

MessagingServer::MessagingServer(const AppConfig &config)
    : server_context(std::make_shared<ServerContext>(config)) {
  dispatcher.registerHandler(MessageType::Text,
                             std::make_unique<ServerTextHandler>());

  dispatcher.registerHandler(MessageType::Command,
                             std::make_unique<ServerCommandHandler>());

  dispatcher.registerHandler(MessageType::CipherMessage,
                             std::make_unique<ServerCipherHandler>());

  dispatcher.registerHandler(MessageType::FileStart,
                             std::make_unique<ServerFileStartHandler>());

  dispatcher.registerHandler(MessageType::FileChunk,
                             std::make_unique<ServerFileChunkHandler>());

  dispatcher.registerHandler(MessageType::FileEnd,
                             std::make_unique<ServerFileEndHandler>());
}

void MessagingServer::start_server() {
  server_context->transport_server->start(server_context->config.server_host,
                                          server_context->config.server_port);
  server_context->transport_server->set_data_callback(
      [this](int fd, auto data) { on_tcp_data_received(fd, data); },
      [this](int fd) {
        server_context->fd = fd;
        dispatcher.setContext(server_context);
        dispatcher.dispatch(Message({}, 1,
                                    {{static_cast<uint8_t>(CommandType::EXIT)}},
                                    MessageType::Command));
      });
}

void MessagingServer::run() {
  server_context->transport_server->run_event_loop();
}

void MessagingServer::stop() {
  if (server_context && server_context->transport_server) {
    server_context->transport_server->stop();
  }
}

void MessagingServer::on_tcp_data_received(int fd,
                                           std::vector<uint8_t> &raw_data) {
  Message msg = server_context->serializer.deserialize(raw_data);

  server_context->fd = fd;
  dispatcher.setContext(server_context);
  dispatcher.dispatch(msg);
}

void MessagingServer::send_error(int fd, const std::string & /*error_string*/) {
  server_context->transport_server->send(fd, {'e', 'r', 'r', 'o', 'r'});
}
