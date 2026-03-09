#include "../../../include/services/messaging_peer.hpp"

MessagingPeer::MessagingPeer(int port)
    : context_(std::make_shared<PeerContext>()) {
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

  start_listening(*context_->peer_node, port);
  context_->peer_node->callbacks_.on_peer_connected = ([](int fd) {
    std::cout << "Peer with file descriptor " << fd << " was connected"
              << std::endl;
  });
  context_->peer_node->callbacks_.on_data_callback =
      ([&](int fd, const std::vector<uint8_t> &raw_data) {
        Message msg = context_->serializer->deserialize(raw_data);
        context_->fd = fd;
        dispatcher.setContext(context_);
        dispatcher.dispatchReceiving(msg);
      });
}

void MessagingPeer::run() { run_event_loop(*context_->peer_node); }

void MessagingPeer::stop_peer() {
  // context_->peer_node->running_ = false;
  stop(*context_->peer_node);
}

void MessagingPeer::send_msg(const std::string &msg_to_send) {
  Message msg = context_->parser.parse(msg_to_send);
  dispatcher.setContext(context_);
  dispatcher.dispatchSending(msg);
}

MessagingPeer::~MessagingPeer() { stop(*context_->peer_node); }
