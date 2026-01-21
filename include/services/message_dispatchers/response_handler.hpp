#pragma once
#include "../encryption_service.hpp"
#include "../message_dispatcher.hpp"
#include "../message_queue.hpp"

class ResponseMessageHandler : public IMessageHandler {
public:
  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override {
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
      auto other_pubkey = IdentityKey::from_public_bytes(msg.get_payload());
      auto username = msg.get_meta(1);

      std::cout << "You got a public key of "
                << std::string(username.begin(), username.end()) << std::endl;
      context->encryption_service->cache_public_key(username,
                                                    msg.get_payload());

      auto pending_it = context->mq->find_pending(username);
      if (pending_it) {
        auto msg_to_send = *pending_it;
        auto ciphertext = context->encryption_service->encrypt_for(
            context->my_username, username, msg_to_send.bytes);
        Message cipher_msg{ciphertext,
                           2,
                           {msg_to_send.recipient_id, context->my_username},
                           MessageType::CipherMessage};

        context->client->send_to_server(
            context->serializer->serialize(cipher_msg));

        std::cout << "[Crypto] Sent queued message to "
                  << std::string(username.begin(), username.end()) << std::endl;
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
        context->encryption_service->cache_public_key(username,
                                                      msg.get_payload());
        auto plaintext = context->encryption_service->decrypt_for(
            context->my_username, context->my_username, pending_ciphertext);
        auto plain_str = std::string(plaintext.begin(), plaintext.end());
        std::cout << plain_str << std::endl;
      }
      break;
    }
    }
  }

  void handleMessageOnServer(const Message &msg,
                             std::shared_ptr<ServerContext> context) override {}

  MessageType getMessageType() const override { return MessageType::Response; }
};
