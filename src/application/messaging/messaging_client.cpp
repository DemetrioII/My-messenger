#include "include/application/messaging/messaging_client.hpp"

void MessagingClient::handle_command(const std::string &msg) {
  ParsedCommand cmd_struct = context->parser.parse_line(msg);
}

void MessagingClient::dispatch_incoming(const Message &msg) {
  dispatcher.setContext(context);
  dispatcher.dispatch(msg);
}

void MessagingClient::dispatch_outgoing(const Message &msg) {
  dispatcher.setContext(context);
  dispatcher.dispatch(msg);
}

MessagingClient::MessagingClient(const AppConfig &config)
    : context(std::make_shared<ClientContext>(config)) {
  dispatcher.registerHandler(MessageType::Text,
                             std::make_unique<ClientTextHandler>());
  dispatcher.registerHandler(MessageType::Command,
                             std::make_unique<ClientCommandHandler>());
  dispatcher.registerHandler(MessageType::Response,
                             std::make_unique<ResponseMessageHandler>());
  dispatcher.registerHandler(MessageType::CipherMessage,
                             std::make_unique<CipherMessageHandler>());
  dispatcher.registerHandler(MessageType::FileStart,
                             std::make_unique<ClientFileStartHandler>());
  dispatcher.registerHandler(MessageType::FileChunk,
                             std::make_unique<ClientFileChunkHandler>());
  dispatcher.registerHandler(MessageType::FileEnd,
                             std::make_unique<ClientFileEndHandler>());
}

int MessagingClient::init_client() {
  context->client->set_data_callback(
      [this](const std::vector<uint8_t> &data) { on_tcp_data_received(data); });
  return 1;
}

void MessagingClient::on_tcp_data_received(
    const std::vector<uint8_t> &raw_data) {
  Message msg = context->serializer->deserialize(raw_data);
  dispatch_incoming(msg);
  // context->ui_callback(msg.get_payload());
}

std::shared_ptr<ClientContext> MessagingClient::get_context() {
  return context;
}

std::vector<std::string>
MessagingClient::search_online_users(const std::string & /*pattern*/) {
  return {};
}

std::optional<std::vector<std::vector<uint8_t>>>
MessagingClient::query_user_info(std::string & /*username*/) {
  return std::nullopt;
}

void MessagingClient::get_data(const std::string &data) {
  Message msg = context->parser.parse(data);
  dispatch_outgoing(msg);
  // context->ui_callback(msg.get_payload());
}

void MessagingClient::stop() {
  if (context && context->client) {
    context->client->disconnect();
  }
}

void MessagingClient::run() { context->client->run_event_loop(); }
