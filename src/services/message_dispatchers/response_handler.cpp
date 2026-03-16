#include "../../../include/services/message_dispatchers/response_handler.hpp"

void ResponseMessageHandler::handleMessageOnClient(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  auto meta0 = msg.get_meta(0);
  if (meta0.empty())
    return;

  auto cmd_type = static_cast<CommandType>(meta0[0]);
  switch (cmd_type) {
  case CommandType::GET_ID: {
    auto my_username = msg.get_meta(1);
    std::string username(my_username.begin(), my_username.end());
    std::cout << "Your username is " << username << std::endl;
    break;
  }

  case CommandType::GET_PUBKEY: {
    auto other_pubkey = DH_Key::from_public_bytes(msg.get_payload());
    auto other_identity_key = IdentityKey::from_public_bytes(msg.get_meta(2));
    auto username = msg.get_meta(1);

    std::cout << "You got a public key of "
              << std::string(username.begin(), username.end()) << std::endl;
    context->encryption_service->cache_public_key(username, msg.get_payload(),
                                                  msg.get_meta(2));

    auto pending_it = context->mq->find_pending(username);
    while (pending_it) {
      auto msg_to_send = *pending_it;

      auto counter_it = context->messages_counter.find(username);
      if (counter_it == context->messages_counter.end()) {
        context->messages_counter[username] = 0;
      }
      auto ciphertext = context->encryption_service->encrypt_for(
          context->my_username, username, msg_to_send.bytes,
          context->messages_counter[username]);

      Message cipher_msg{std::move(ciphertext), // ?
                         2,
                         {username, context->my_username},
                         MessageType::CipherMessage};

      context->client->send_to_server(
          context->serializer->serialize(cipher_msg));

      ++context->messages_counter[username];

      std::cout << "[Crypto] Sent queued message to "
                << std::string(username.begin(), username.end()) << std::endl;

      pending_it = context->mq->find_pending(username);
    }

    std::vector<std::vector<uint8_t>> pending_received;
    {
      std::lock_guard<std::mutex> lock(context->pending_messages_mutex);
      auto it = context->pending_messages->find(username);
      if (it != context->pending_messages->end()) {
        pending_received = std::move(it->second);
        context->pending_messages->erase(it);
      }
    }

    for (const auto &pending_ciphertext : pending_received) {
      context->encryption_service->cache_public_key(username, msg.get_payload(),
                                                    msg.get_meta(2));
      auto plaintext = context->encryption_service->decrypt_for(
          context->my_username, context->my_username, pending_ciphertext,
          context->encryption_service->sign(),
          context->messages_counter.find(username)->second);
      auto plain_str = std::string(plaintext.begin(), plaintext.end());
      std::cout << plain_str << std::endl;
    }
    break;
  }
  }
}

void ResponseMessageHandler::handleMessageOnServer(
    const Message &msg, std::shared_ptr<ServerContext> context) {}

void ResponseMessageHandler::handleOnRecvPeer(
    const Message &msg, std::shared_ptr<PeerContext> context) {
  auto meta0 = msg.get_meta(0);
  if (meta0.empty())
    return;

  auto cmd_type = static_cast<CommandType>(meta0[0]);
  switch (cmd_type) {
  case CommandType::GET_PUBKEY: {
    std::string username_str =
        std::string(msg.get_meta(1).begin(), msg.get_meta(1).end());
    std::cout << "You got a public key of " << username_str << std::endl;
    break;
  }
  case CommandType::CONNECT: {
    std::string username_str =
        std::string(msg.get_payload().begin(), msg.get_payload().end());
    const std::vector<uint8_t> &DH_bytes = msg.get_meta(1);
    const std::vector<uint8_t> &identity_bytes = msg.get_meta(2);
    const std::vector<uint8_t> &signature = msg.get_meta(3);
    auto register_res = context->user_service->register_user(
        username_str, DH_bytes, identity_bytes, signature);
    if (!register_res.has_value()) {
      switch (register_res.error()) {
      case ServiceError::AlreadyExists: {
        std::cout << "This user is already exist" << std::endl;
        return;
      }

      default: {
        std::cout << "Unknown error" << std::endl;
        return;
      }
      }
    }
    context->session_manager->bind(context->fd, username_str);
    context->encryption_service->cache_public_key(msg.get_payload(), DH_bytes,
                                                  identity_bytes);
    std::cout << "You got a public keys of " << username_str << std::endl;
    break;
  }
  }
}

void ResponseMessageHandler::handleOnSendPeer(
    const Message &msg, std::shared_ptr<PeerContext> context) {}

MessageType ResponseMessageHandler::getMessageType() const {
  return MessageType::Response;
}
