#include <gtest/gtest.h>

#include "include/transport/peer.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

class P2PNodeHarness {
public:
  explicit P2PNodeHarness(int port)
      : node(PeerNodeFactory::create_tcp_peer()), listen_port(port) {
    start_listening(*node, listen_port);
    loop_thread = std::thread([this]() { run_event_loop(*node); });
  }

  ~P2PNodeHarness() { Stop(); }

  P2PNodeHarness(const P2PNodeHarness &) = delete;
  P2PNodeHarness &operator=(const P2PNodeHarness &) = delete;

  std::shared_ptr<PeerNode> get() const { return node; }

  void Stop() {
    if (stopped.exchange(true))
      return;
    stop(*node);
    if (loop_thread.joinable())
      loop_thread.join();
  }

private:
  std::shared_ptr<PeerNode> node;
  int listen_port;
  std::thread loop_thread;
  std::atomic<bool> stopped{false};
};

class PeerNodeTest : public ::testing::Test {
protected:
  int nextPort() {
    static std::atomic<int> counter{0};
    return 9000 + (counter++ % 100);
  }
};

TEST_F(PeerNodeTest, CreateAndDestroyPeerNode) {
  auto peer = PeerNodeFactory::create_tcp_peer();
  ASSERT_NE(peer, nullptr);
  EXPECT_FALSE(peer->running_);
}

TEST_F(PeerNodeTest, StartAndStopListening) {
  P2PNodeHarness server(nextPort());
  EXPECT_TRUE(server.get()->running_);
  server.Stop();
  EXPECT_FALSE(server.get()->running_);
}

TEST_F(PeerNodeTest, ConnectAndDisconnectPeer) {
  int port = nextPort();
  P2PNodeHarness server(port);
  auto client = PeerNodeFactory::create_tcp_peer();

  ASSERT_TRUE(connect_to_peer(*client, "127.0.0.1", port));
  std::this_thread::sleep_for(100ms);
  // EXPECT_EQ(get_active_connections_count(*client), 1u);

  stop(*client);
  server.Stop();
}

TEST_F(PeerNodeTest, BroadcastMessage) {
  int port = nextPort();
  P2PNodeHarness server(port);
  auto client1 = PeerNodeFactory::create_tcp_peer();
  auto client2 = PeerNodeFactory::create_tcp_peer();

  std::atomic<int> received{0};
  auto handler = [&](int, const std::vector<uint8_t> &data) {
    std::string msg(data.begin(), data.end());
    if (msg == "hello")
      ++received;
  };

  client1->callbacks_.on_data_callback = handler;
  client2->callbacks_.on_data_callback = handler;

  ASSERT_TRUE(connect_to_peer(*client1, "127.0.0.1", port));
  ASSERT_TRUE(connect_to_peer(*client2, "127.0.0.1", port));
  std::this_thread::sleep_for(200ms);

  std::vector<uint8_t> payload{'h', 'e', 'l', 'l', 'o'};
  broadcast(*client2, payload);

  std::this_thread::sleep_for(300ms);
  EXPECT_EQ(received.load(), 2);

  stop(*client1);
  stop(*client2);
  server.Stop();
}
