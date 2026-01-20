#pragma once
#include "../encryption_service.hpp"
#include "../message_dispatcher.hpp"
#include "../message_queue.hpp"

class ResponseMessageHandler : public IMessageHandler {
  std::vector<uint8_t> my_username;
  std::shared_ptr<EncryptionService> encryption_service;
  std::shared_ptr<MessageQueue> mq;
  std::shared_ptr<IClient> client;
  std::shared_ptr<Serializer> serializer;

  std::mutex pending_messages_mutex;
  std::unordered_map<std::vector<uint8_t>, std::vector<std::vector<uint8_t>>>
      pending_messages;

public:
  ResponseMessageHandler(std::vector<uint8_t> my_username,
                         std::shared_ptr<EncryptionService> encryption_service,
                         std::shared_ptr<MessageQueue> mq,
                         std::shared_ptr<IClient> client,
                         std::shared_ptr<Serializer> serializer)
      : my_username(my_username), encryption_service(encryption_service),
        mq(mq), client(client), serializer(serializer) {}

  void
  handleMessageOnClient(const Message &msg,
                        const std::shared_ptr<ClientContext> context) override {
    auto meta0 = msg.get_meta(0);
    if (meta0.empty())
      return;

    auto cmd_type = static_cast<CommandType>(meta0[0]);

    switch (cmd_type) {
    case CommandType::GET_ID: {
      my_username = msg.get_meta(1);
      std::string username(my_username.begin(), my_username.end());
      std::cout << "Your username is " << username << std::endl;
      break;
    }

    case CommandType::GET_PUBKEY: {
      std::cout << "You got a public key" << std::endl;
      auto other_pubkey = IdentityKey::from_public_bytes(msg.get_payload());
      auto username = msg.get_meta(1);
      encryption_service->cache_public_key(username, msg.get_payload());

      auto pending_it = mq->find_pending(username);
      if (pending_it) {
        auto msg_to_send = *pending_it;
        auto ciphertext =
            encryption_service->encrypt_for(username, msg_to_send.bytes);
        Message cipher_msg{ciphertext,
                           2,
                           {msg_to_send.recipient_id, my_username},
                           MessageType::CipherMessage};

        client->send_to_server(serializer->serialize(cipher_msg));

        std::cout << "[Crypto] Sent queued message to "
                  << std::string(username.begin(), username.end()) << std::endl;
      }

      std::vector<std::vector<uint8_t>> pending_received;
      {
        std::lock_guard<std::mutex> lock(pending_messages_mutex);
        auto it = pending_messages.find(username);
        if (it != pending_messages.end()) {
          pending_received = std::move(it->second);
          pending_messages.erase(it);
        }
      }

      for (const auto &pending_ciphertext : pending_received) {
        encryption_service->cache_public_key(username, msg.get_payload());
        auto plaintext =
            encryption_service->decrypt_for(my_username, pending_ciphertext);
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
