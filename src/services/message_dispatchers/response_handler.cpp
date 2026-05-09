#include "../../../include/services/message_dispatchers/response_handler.hpp"

void ResponseMessageHandler::handle_get_pubkey_response(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  auto username = msg.get_meta(1);

  std::cout << "You got a public key of "
            << std::string_view(reinterpret_cast<const char *>(username.data()),
                                username.size())
            << std::endl;

  context->encryption_service->cache_public_key(username, msg.get_payload(),
                                                msg.get_meta(2));

  auto pending_it = context->mq->find_pending(username);
  while (pending_it) {
    auto msg_to_send = *pending_it;

    if (context->messages_counter.find(username) ==
        context->messages_counter.end()) {
      context->messages_counter[username] = 0;
    }

    auto ciphertext = context->encryption_service->encrypt_for(
        context->my_username, username, msg_to_send.bytes,
        context->messages_counter[username]);

    Message cipher_msg{std::move(ciphertext),
                       2,
                       {username, context->my_username},
                       MessageType::CipherMessage};

    context->client->send_to_server(context->serializer->serialize(cipher_msg));
    ++context->messages_counter[username];

    std::cout << "[Crypto] Sent queued message to "
              << std::string_view(
                     reinterpret_cast<const char *>(username.data()),
                     username.size())
              << std::endl;

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
    auto plaintext = context->encryption_service->decrypt_for(
        context->my_username, context->my_username, pending_ciphertext,
        context->encryption_service->sign(),
        context->messages_counter.find(username)->second);
    auto plain_str = std::string_view(
        reinterpret_cast<const char *>(plaintext.data()), plaintext.size());
    std::cout << plain_str << std::endl;
  }
}

void ResponseMessageHandler::handle_connect_response(
    const Message &msg, const std::shared_ptr<PeerContext> context) {
  std::string username_str(msg.get_payload().begin(), msg.get_payload().end());
  const auto &dh_bytes = msg.get_meta(1);
  const auto &identity_bytes = msg.get_meta(2);
  const auto &signature = msg.get_meta(3);

  auto register_res = context->user_service->register_user(
      username_str, dh_bytes, identity_bytes, signature);
  if (!register_res.has_value()) {
    switch (register_res.error()) {
    case ServiceError::AlreadyExists:
      std::cout << "This user already exists" << std::endl;
      return;
    default:
      std::cout << "Unknown error" << std::endl;
      return;
    }
  }

  context->session_manager->bind(context->fd, username_str);
  context->encryption_service->cache_public_key(msg.get_payload(), dh_bytes,
                                                identity_bytes);
  std::cout << "You got public keys of " << username_str << std::endl;
}

void ResponseMessageHandler::handleIncoming(
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
    handle_get_pubkey_response(msg, context);
    break;
  }
  }
}

void ResponseMessageHandler::handleOutgoing(
    const Message &msg, const std::shared_ptr<ClientContext> context) {
  handleIncoming(msg, context);
}

MessageType ResponseMessageHandler::getMessageType() const {
  return MessageType::Response;
}
