#include "../../../include/services/commands_impl/sendfile.hpp"

CommandType SendFileCommand::getType() const { return CommandType::SEND_FILE; }

void SendFileCommand::fromParsedCommand(const ParsedCommand &pc) {
  if (pc.args.size() >= 2) {
    filename = std::string(pc.args[1].begin(), pc.args[1].end());
    recipient = pc.args[0];
  }
}

Message SendFileCommand::toMessage() const {
  auto fname_bytes = std::vector<uint8_t>(filename.begin(), filename.end());
  return Message(
      {}, 1,
      {{static_cast<uint8_t>(CommandType::SEND_FILE)}, recipient, fname_bytes},
      MessageType::Command);
}

void SendFileCommand::execeuteOnServer(std::shared_ptr<ServerContext> context) {
  std::string response_string = "Server should send a file '" + filename +
                                "' to " +
                                std::string(recipient.begin(), recipient.end());
  context->transport_server->send(
      context->fd,
      context->serializer.serialize(Message(
          std::vector<uint8_t>(response_string.begin(), response_string.end()),
          0, {}, MessageType::Text)));
}

void SendFileCommand::executeOnClient(std::shared_ptr<ClientContext> context) {
  client = context->client;
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot open file: " << filename << std::endl;
    return;
  }

  file.seekg(0, std::ios::end);
  uint64_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> fname_bytes(filename.begin(), filename.end());

  Message start_msg({}, 3, {recipient, fname_bytes, to_bytes(file_size)},
                    MessageType::FileStart);

  client->send_to_server(serializer.serialize(start_msg));

  const size_t CHUNK = 16 * 1024;
  std::vector<uint8_t> buffer(CHUNK);

  while (file) {
    file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
    auto read_bytes = file.gcount();
    if (read_bytes == 0)
      break;

    auto cipher_bytes = context->encryption_service->encrypt_for(
        context->my_username, recipient, buffer);

    std::vector<uint8_t> chunk(cipher_bytes.begin(), cipher_bytes.end());

    Message chunk_msg(chunk, 3, {recipient, fname_bytes, context->my_username},
                      MessageType::FileChunk);

    client->send_to_server(serializer.serialize(chunk_msg));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  Message end_msg({}, 2, {recipient, fname_bytes}, MessageType::FileEnd);

  client->send_to_server(serializer.serialize(end_msg));

  std::cout << "[File] Sent: " << filename << " (" << file_size << " bytes)"
            << std::endl;
}

void SendFileCommand::fromMessage(const Message &msg) {
  auto fname_bytes = msg.get_meta(2);
  filename = std::string(fname_bytes.begin(), fname_bytes.end());
  recipient = msg.get_meta(1);
}

bool SendFileCommand::isClientOnly() const { return true; }

SendFileCommand::~SendFileCommand() {}
