#include "../../../include/services/messaging_client.hpp"

void MessagingClient::handle_command(const std::string &msg) {
  ParsedCommand cmd_struct = context->parser.parse_line(msg);
}

MessagingClient::MessagingClient(const std::string &server_ip, int port)
    : context(std::make_shared<ClientContext>(server_ip, port)) {
  dispatcher.registerHandler(MessageType::Text,
                             std::make_unique<TextMessageHandler>());
  dispatcher.registerHandler(MessageType::Command,
                             std::make_unique<MessageCommandHandler>());
  dispatcher.registerHandler(MessageType::Response,
                             std::make_unique<ResponseMessageHandler>());
  dispatcher.registerHandler(MessageType::CipherMessage,
                             std::make_unique<CipherMessageHandler>());
  dispatcher.registerHandler(MessageType::FileStart,
                             std::make_unique<FileStartHandler>());
  dispatcher.registerHandler(MessageType::FileChunk,
                             std::make_unique<FileChunkHandler>());
  dispatcher.registerHandler(MessageType::FileEnd,
                             std::make_unique<FileEndHandler>());
}

int MessagingClient::init_client(const std::string &server_ip, int port) {
  context->client->set_data_callback(
      [this](const std::vector<uint8_t> &data) { on_tcp_data_received(data); });
  return 1;
}

void MessagingClient::on_tcp_data_received(
    const std::vector<uint8_t> &raw_data) {
  Message msg = context->serializer->deserialize(raw_data);
  dispatcher.setContext(context);
  dispatcher.dispatch(msg);
  // context->ui_callback(msg.get_payload());
}

std::shared_ptr<ClientContext> MessagingClient::get_context() {
  return context;
}

std::vector<std::string>
MessagingClient::search_online_users(const std::string &pattern) {}

std::optional<std::vector<std::vector<uint8_t>>>
MessagingClient::query_user_info(std::string &username) {}

void MessagingClient::get_data(const std::string &data) {
  Message msg = context->parser.parse(data);
  auto bytes = context->serializer->serialize(msg);
  if (msg.get_type() == MessageType::Command) {
    dispatcher.setContext(context);
    dispatcher.dispatch(msg);
  } else if (msg.get_type() == MessageType::Text) {
    context->client->send_to_server(bytes);
  }
  // context->ui_callback(msg.get_payload());
}

void MessagingClient::run() { context->client->run_event_loop(); }
