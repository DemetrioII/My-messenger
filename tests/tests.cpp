#include <gtest/gtest.h>

#include "include/app/config.hpp"
#include "include/application/services/chat_service.hpp"
#include "include/application/services/server_application_service.hpp"
#include "include/application/services/session_manager.hpp"
#include "include/application/services/user_service.hpp"
#include "include/models/message.hpp"
#include "include/protocol/parser.hpp"
#include "include/transport/interface.hpp"

#include <memory>
#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<uint8_t> bytes(const std::string &value) {
  return {value.begin(), value.end()};
}

std::string as_string(const std::vector<uint8_t> &value) {
  return {value.begin(), value.end()};
}

class FakeServer final : public IServer {
public:
  std::vector<std::pair<int, std::vector<uint8_t>>> sent;
  std::function<void(int, std::vector<uint8_t>)> data_callback;
  std::function<void(int)> disconnect_callback;
  bool running = false;

  void start(const std::string &, int) override { running = true; }
  void stop() override { running = false; }
  bool is_running() const override { return running; }
  void on_client_error(int) override {}
  void on_client_connected(std::shared_ptr<IConnection>) override {}
  void on_client_disconnected(int) override {}
  void on_client_message(int, const std::vector<uint8_t> &) override {}
  void on_client_writable(int) override {}
  void send(int fd, const std::vector<uint8_t> &data) override {
    sent.emplace_back(fd, data);
  }
  void set_data_callback(std::function<void(int, std::vector<uint8_t>)> callback,
                         std::function<void(int)> disconnect) override {
    data_callback = std::move(callback);
    disconnect_callback = std::move(disconnect);
  }
  void run_event_loop() override {}
  void tls_handshake(int) override {}
  bool tls_handshake_done(int) override { return true; }
};

class ParserTest : public ::testing::Test {};
class ServiceTest : public ::testing::Test {};
class ServerAppTest : public ::testing::Test {
protected:
  void SetUp() override {
    fd = 42;
    transport = std::make_shared<FakeServer>();
    user_service = std::make_shared<UserService>();
    chat_service = std::make_shared<ChatService>();
    session_manager = std::make_shared<SessionManager>();
    service = std::make_unique<ServerApplicationService>(
        user_service, chat_service, session_manager, transport, &serializer,
        &fd);
  }

  std::shared_ptr<FakeServer> transport;
  std::shared_ptr<UserService> user_service;
  std::shared_ptr<ChatService> chat_service;
  std::shared_ptr<SessionManager> session_manager;
  Serializer serializer;
  int fd = -1;
  std::unique_ptr<ServerApplicationService> service;
};

} // namespace

TEST_F(ParserTest, TextMessageRoundTrip) {
  Parser parser;
  auto msg = parser.parse("hello messenger");

  ASSERT_EQ(msg.get_type(), MessageType::Text);
  EXPECT_EQ(as_string(msg.get_payload()), "hello messenger");

  Serializer serializer;
  auto raw = serializer.serialize(msg);
  auto restored = serializer.deserialize(raw);

  EXPECT_EQ(restored.get_type(), MessageType::Text);
  EXPECT_EQ(as_string(restored.get_payload()), "hello messenger");
}

TEST_F(ParserTest, CommandParsingAndReverseMapping) {
  Parser parser;
  auto msg = parser.parse("/login alice");

  ASSERT_EQ(msg.get_type(), MessageType::Command);
  ASSERT_FALSE(msg.get_meta(0).empty());
  EXPECT_EQ(static_cast<CommandType>(msg.get_meta(0)[0]), CommandType::LOGIN);

  auto parsed = parser.make_struct_from_command(msg);
  EXPECT_EQ(parsed.name, "login");
  ASSERT_EQ(parsed.args.size(), 1u);
  EXPECT_EQ(as_string(parsed.args[0]), "alice");
}

TEST_F(ParserTest, SerializerPreservesCommandMetadata) {
  Parser parser;
  auto msg = parser.parse("/room general");

  Serializer serializer;
  auto raw = serializer.serialize(msg);
  auto restored = serializer.deserialize(raw);
  auto parsed = parser.make_struct_from_command(restored);

  EXPECT_EQ(parsed.name, "room");
  ASSERT_EQ(parsed.args.size(), 1u);
  EXPECT_EQ(as_string(parsed.args[0]), "general");
}

TEST_F(ServiceTest, UserServiceRegistersFindsAndRemovesUser) {
  UserService users;

  auto reg = users.register_user("alice", bytes("dh"), bytes("id"), bytes("sig"));
  ASSERT_TRUE(reg.has_value());
  EXPECT_EQ(*reg, "alice");

  auto dup = users.register_user("alice", bytes("dh2"), bytes("id2"), bytes("sig2"));
  ASSERT_FALSE(dup.has_value());
  EXPECT_EQ(dup.error(), ServiceError::AlreadyExists);

  auto found = users.find_user("alice");
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ((*found)->get_username(), "alice");
  EXPECT_EQ((*found)->get_public_DH_key(), bytes("dh"));
  EXPECT_EQ((*found)->get_public_Identity_key(), bytes("id"));
  EXPECT_EQ((*found)->get_key_signature(), bytes("sig"));

  users.remove_user("alice");
  auto missing = users.find_user("alice");
  ASSERT_FALSE(missing.has_value());
  EXPECT_EQ(missing.error(), ServiceError::UserNotFound);
}

TEST_F(ServiceTest, ChatServiceCreatesMembersAndBroadcastRecipients) {
  ChatService chats;

  auto created = chats.create_chat("general", "alice");
  ASSERT_TRUE(created.has_value());
  EXPECT_EQ(*created, "general");

  auto added = chats.add_member("general", "bob");
  ASSERT_TRUE(added.has_value());

  auto duplicate = chats.add_member("general", "bob");
  ASSERT_FALSE(duplicate.has_value());
  EXPECT_EQ(duplicate.error(), ServiceError::AlreadyMember);

  Message msg(bytes("hello"), 0, {}, MessageType::Text);
  auto recipients = chats.post_message("general", "alice", msg);
  ASSERT_TRUE(recipients.has_value());
  ASSERT_EQ(recipients->size(), 2u);
  EXPECT_NE(std::find(recipients->begin(), recipients->end(), "alice"),
            recipients->end());
  EXPECT_NE(std::find(recipients->begin(), recipients->end(), "bob"),
            recipients->end());
}

TEST_F(ServiceTest, SessionManagerBindsAndUnbindsDescriptors) {
  SessionManager sessions;

  sessions.bind(10, "alice");

  auto username = sessions.get_username(10);
  ASSERT_TRUE(username.has_value());
  EXPECT_EQ(*username, "alice");

  auto fd = sessions.get_fd("alice");
  ASSERT_TRUE(fd.has_value());
  EXPECT_EQ(*fd, 10);

  sessions.unbind(10);

  auto missing_username = sessions.get_username(10);
  ASSERT_FALSE(missing_username.has_value());
  EXPECT_EQ(missing_username.error(), ServiceError::UserNotFound);
}

TEST_F(ServerAppTest, LoginBindsSessionAndSendsGreeting) {
  service->login("alice", bytes("dh"), bytes("identity"), bytes("sig"));

  ASSERT_EQ(transport->sent.size(), 1u);
  EXPECT_EQ(transport->sent.back().first, fd);

  auto reply = serializer.deserialize(transport->sent.back().second);
  EXPECT_EQ(reply.get_type(), MessageType::Text);
  EXPECT_EQ(as_string(reply.get_payload()), "Hello, alice!");

  auto current = service->current_username();
  ASSERT_TRUE(current.has_value());
  EXPECT_EQ(*current, "alice");
}

TEST_F(ServerAppTest, LoginTwiceReturnsAlreadyLoggedInResponse) {
  service->login("alice", bytes("dh"), bytes("identity"), bytes("sig"));
  transport->sent.clear();

  service->login("alice", bytes("dh2"), bytes("identity2"), bytes("sig2"));

  ASSERT_EQ(transport->sent.size(), 1u);
  EXPECT_EQ(transport->sent.back().second, StaticResponses::YOU_ARE_LOGGED_IN);
}

TEST_F(ServerAppTest, JoinRequiresLoginAndThenSucceeds) {
  service->join_room("general");
  ASSERT_EQ(transport->sent.size(), 1u);
  EXPECT_EQ(transport->sent.back().second, StaticResponses::YOU_NEED_TO_LOGIN);

  transport->sent.clear();
  service->login("alice", bytes("dh"), bytes("identity"), bytes("sig"));
  transport->sent.clear();

  service->create_room("general");
  transport->sent.clear();

  fd = 7;
  session_manager->bind(fd, "bob");
  user_service->register_user("bob", bytes("dh_b"), bytes("id_b"), bytes("sig_b"));
  service->join_room("general");
  ASSERT_EQ(transport->sent.size(), 1u);

  auto reply = serializer.deserialize(transport->sent.back().second);
  EXPECT_EQ(reply.get_type(), MessageType::Text);
  EXPECT_EQ(as_string(reply.get_payload()), "You joined the room general");
}

TEST_F(ServerAppTest, CreateRoomAndPreventDuplicates) {
  service->login("alice", bytes("dh"), bytes("identity"), bytes("sig"));
  transport->sent.clear();

  service->create_room("general");
  ASSERT_EQ(transport->sent.size(), 1u);
  auto created = serializer.deserialize(transport->sent.back().second);
  EXPECT_EQ(created.get_type(), MessageType::Text);
  EXPECT_EQ(as_string(created.get_payload()), "Room general was created!");

  transport->sent.clear();
  service->create_room("general");
  ASSERT_EQ(transport->sent.size(), 1u);
  EXPECT_EQ(transport->sent.back().second, StaticResponses::CHAT_ALREADY_EXISTS);
}

TEST_F(ServerAppTest, GroupMessageBroadcastsToMembers) {
  service->login("alice", bytes("dh_a"), bytes("id_a"), bytes("sig_a"));
  transport->sent.clear();
  session_manager->bind(7, "bob");
  user_service->register_user("bob", bytes("dh_b"), bytes("id_b"), bytes("sig_b"));
  service->create_room("general");
  chat_service->add_member("general", "bob");
  transport->sent.clear();

  Message group_msg(bytes("hello group"), 0, {}, MessageType::Text);
  service->send_group_message("general", group_msg);

  ASSERT_EQ(transport->sent.size(), 2u);
  EXPECT_EQ(transport->sent[0].first, fd);
  EXPECT_EQ(transport->sent[1].first, 7);

  auto sent0 = serializer.deserialize(transport->sent[0].second);
  auto sent1 = serializer.deserialize(transport->sent[1].second);
  EXPECT_EQ(as_string(sent0.get_payload()), "hello group");
  EXPECT_EQ(as_string(sent1.get_payload()), "hello group");
}

TEST_F(ServerAppTest, PubkeyLookupReturnsStoredKeys) {
  service->login("alice", bytes("dh_a"), bytes("id_a"), bytes("sig_a"));
  transport->sent.clear();

  service->send_pubkey(bytes("alice"));

  ASSERT_EQ(transport->sent.size(), 1u);
  auto reply = serializer.deserialize(transport->sent.back().second);
  EXPECT_EQ(reply.get_type(), MessageType::Response);
  ASSERT_FALSE(reply.get_meta(0).empty());
  EXPECT_EQ(static_cast<CommandType>(reply.get_meta(0)[0]),
            CommandType::GET_PUBKEY);
  EXPECT_EQ(as_string(reply.get_meta(1)), "alice");
}

TEST_F(ServerAppTest, CipherMessageIsForwardedToRecipient) {
  service->login("alice", bytes("dh_a"), bytes("id_a"), bytes("sig_a"));
  transport->sent.clear();

  user_service->register_user("bob", bytes("dh_b"), bytes("id_b"), bytes("sig_b"));
  session_manager->bind(7, "bob");

  Message cipher_msg(bytes("ciphertext"), 2,
                     {bytes("bob"), bytes("alice")}, MessageType::CipherMessage);
  service->deliver_cipher_message(fd, cipher_msg);

  ASSERT_EQ(transport->sent.size(), 1u);
  EXPECT_EQ(transport->sent.back().first, 7);

  auto reply = serializer.deserialize(transport->sent.back().second);
  EXPECT_EQ(reply.get_type(), MessageType::CipherMessage);
  EXPECT_EQ(as_string(reply.get_payload()), "ciphertext");
  EXPECT_EQ(as_string(reply.get_meta(0)), "bob");
  EXPECT_EQ(as_string(reply.get_meta(1)), "alice");
}
