#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../include/network/transport/peer.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

// Мок-объект для ISocket (опционально, но полезно для изолированного
// тестирования)
class MockSocket : public ISocket {
public:
  MOCK_METHOD(int, create_socket, (), (override));
  MOCK_METHOD(int, setup_server, (int port, const std::string &ip), (override));
  MOCK_METHOD(int, setup_connection, (const std::string &ip, int port),
              (override));
  MOCK_METHOD(void, make_address_reusable, (), (override));
  MOCK_METHOD(int, get_fd, (), (const override));
  MOCK_METHOD(SocketType, get_type, (), (const override));
  MOCK_METHOD(void, close, (), (override));
  MOCK_METHOD(sockaddr_in, get_peer_address, (), (const override));
  MOCK_METHOD(int, bind, (const sockaddr_in &addr), (override));
  MOCK_METHOD(int, listen, (int backlog), (override));
  MOCK_METHOD(int, accept, (sockaddr_in & client_addr), (override));
};

// Фикстура для тестов
class PeerNodeTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Даем время на освобождение портов между тестами
    std::this_thread::sleep_for(50ms);
  }

  void TearDown() override {
    // Даем время на корректное завершение
    std::this_thread::sleep_for(50ms);
  }

  // Вспомогательная функция для запуска узла в отдельном потоке
  std::thread startPeerInThread(std::shared_ptr<PeerNode> peer, int port) {
    return std::thread([peer, port]() {
      try {
        peer->start_listening(port);
        peer->run_event_loop();
      } catch (const std::exception &e) {
        std::cerr << "Error in peer thread: " << e.what() << std::endl;
      }
    });
  }

  // Портовая политика для тестов
  int getTestPort(int base = 9000) {
    static std::atomic<int> counter{0};
    return base + (counter++ % 100); // Диапазон портов для тестов
  }
};

// Тест 1: Создание и уничтожение PeerNode
TEST_F(PeerNodeTest, CreateAndDestroy) {
  auto peer = PeerNodeFactory::create_tcp_peer();
  EXPECT_NE(peer, nullptr);

  // Проверяем начальное состояние
  EXPECT_FALSE(peer->is_running());
  EXPECT_EQ(peer->get_active_connections_count(), 0);
}

// Тест 2: Запуск прослушивания на порту
TEST_F(PeerNodeTest, StartListening) {
  auto peer = PeerNodeFactory::create_tcp_peer();
  int port = getTestPort();

  EXPECT_NO_THROW(peer->start_listening(port));

  // Узел должен быть в состоянии running после запуска
  EXPECT_TRUE(peer->is_running());

  // Должен быть создан сокет для прослушивания
  EXPECT_GT(peer->get_active_connections_count(), 0);

  peer->stop();
}

// Тест 3: Подключение к другому узлу
TEST_F(PeerNodeTest, ConnectToPeer) {
  // Создаем два узла
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();

  int port1 = getTestPort();
  int port2 = getTestPort();

  // Запускаем первый узел
  peer1->start_listening(port1);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms); // Даем время на запуск

  // Подключаем второй узел к первому
  bool connected = peer2->connect_to_peer("127.0.0.1", port1);
  EXPECT_TRUE(connected);

  // Даем время на установление соединения
  std::this_thread::sleep_for(100ms);

  // Проверяем количество соединений
  EXPECT_EQ(peer2->get_active_connections_count(), 1);

  // Останавливаем
  peer1->stop();
  peer2->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 4: Отправка и получение сообщения
TEST_F(PeerNodeTest, SendAndReceiveMessage) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  std::promise<std::string> received_promise;
  std::future<std::string> received_future = received_promise.get_future();

  std::string test_message = "Hello, Peer!";
  std::atomic<bool> message_received{false};

  // Настраиваем обработчик сообщений для peer2
  peer2->set_data_callback(
      [&](const std::string &ip, const std::vector<uint8_t> &data) {
        std::string message(data.begin(), data.end());
        received_promise.set_value(message);
        message_received = true;
      });

  // Запускаем оба узла
  peer1->start_listening(port);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаем peer2 к peer1
  bool connected = peer2->connect_to_peer("127.0.0.1", port);
  ASSERT_TRUE(connected);

  std::this_thread::sleep_for(100ms); // Даем время на соединение

  // Отправляем сообщение от peer1 к peer2
  std::vector<uint8_t> data(test_message.begin(), test_message.end());
  peer1->send_to_peer("127.0.0.1", data);

  // Ждем получения сообщения (с таймаутом)
  auto status = received_future.wait_for(2s);
  ASSERT_EQ(status, std::future_status::ready);

  std::string received = received_future.get();
  EXPECT_EQ(received, test_message);

  // Останавливаем
  peer1->stop();
  peer2->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 5: Широковещательная рассылка
TEST_F(PeerNodeTest, BroadcastMessage) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();
  auto peer3 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  std::atomic<int> messages_received{0};
  std::string test_message = "Broadcast Test";

  // Настраиваем обработчики для peer2 и peer3
  auto message_handler = [&](const std::string &ip,
                             const std::vector<uint8_t> &data) {
    std::string message(data.begin(), data.end());
    if (message == test_message) {
      messages_received++;
    }
  };

  peer2->set_data_callback(message_handler);
  peer3->set_data_callback(message_handler);

  // Запускаем основной узел
  peer1->start_listening(port);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаем два клиента
  bool connected2 = peer2->connect_to_peer("127.0.0.1", port);
  bool connected3 = peer3->connect_to_peer("127.0.0.1", port);

  ASSERT_TRUE(connected2);
  ASSERT_TRUE(connected3);

  std::this_thread::sleep_for(200ms); // Даем время на соединение

  // Отправляем широковещательное сообщение
  std::vector<uint8_t> data(test_message.begin(), test_message.end());
  peer1->broadcast(data);

  // Ждем получения сообщений
  std::this_thread::sleep_for(500ms);

  // Оба клиента должны получить сообщение
  EXPECT_EQ(messages_received.load(), 2);

  // Останавливаем
  peer1->stop();
  peer2->stop();
  peer3->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 6: Отключение от пира
TEST_F(PeerNodeTest, DisconnectFromPeer) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  std::atomic<bool> disconnected{false};

  // Настраиваем callback для отключения
  peer2->set_peer_disconnected_callback(
      [&](const std::string &ip) { disconnected = true; });

  // Запускаем узлы
  peer1->start_listening(port);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаем
  bool connected = peer2->connect_to_peer("127.0.0.1", port);
  ASSERT_TRUE(connected);

  std::this_thread::sleep_for(100ms);

  // Проверяем, что соединение установлено
  EXPECT_EQ(peer2->get_active_connections_count(), 1);

  // Отключаем
  peer2->disconnect_from_peer_by_ip("127.0.0.1");

  // Ждем отключения
  std::this_thread::sleep_for(200ms);

  EXPECT_TRUE(disconnected.load());
  EXPECT_EQ(peer2->get_active_connections_count(), 0);

  // Останавливаем
  peer1->stop();
  peer2->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 7: Получение списка подключенных пиров
TEST_F(PeerNodeTest, GetConnectedPeers) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();
  auto peer3 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  // Запускаем основной узел
  peer1->start_listening(port);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаем два клиента
  peer2->connect_to_peer("127.0.0.1", port);
  peer3->connect_to_peer("127.0.0.1", port);

  std::this_thread::sleep_for(200ms);

  // Проверяем список пиров
  auto peers = peer1->get_connected_peers();

  // Должно быть 2 подключения
  EXPECT_EQ(peers.size(), 2);

  // Проверяем, что IP адреса корректны (127.0.0.1)
  for (const auto &ip : peers) {
    EXPECT_EQ(ip, "127.0.0.1");
  }

  // Останавливаем
  peer1->stop();
  peer2->stop();
  peer3->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 8: Обработка ошибок соединения
TEST_F(PeerNodeTest, ConnectionErrorHandling) {
  auto peer = PeerNodeFactory::create_tcp_peer();

  // Пытаемся подключиться к несуществующему серверу
  bool connected = peer->connect_to_peer("127.0.0.1", 9999);

  // Подключение должно завершиться неудачей
  EXPECT_FALSE(connected);
  EXPECT_EQ(peer->get_active_connections_count(), 0);
}

// Тест 9: Множественные соединения и разрывы
TEST_F(PeerNodeTest, MultipleConnectionsAndDisconnections) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();
  auto peer3 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  std::atomic<int> connection_count{0};
  std::atomic<int> disconnection_count{0};

  // Счетчики событий
  peer1->set_peer_connected_callback(
      [&](const std::string &) { connection_count++; });

  peer1->set_peer_disconnected_callback(
      [&](const std::string &) { disconnection_count++; });

  // Запускаем основной узел
  peer1->start_listening(port);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаем два клиента
  peer2->connect_to_peer("127.0.0.1", port);
  peer3->connect_to_peer("127.0.0.1", port);

  std::this_thread::sleep_for(200ms);

  // Проверяем подключения
  EXPECT_EQ(connection_count.load(), 2);
  EXPECT_EQ(peer1->get_active_connections_count(), 2);

  // Отключаем одного клиента
  peer1->disconnect_from_peer_by_ip("127.0.0.1");

  std::this_thread::sleep_for(200ms);

  // Проверяем отключение
  EXPECT_EQ(disconnection_count.load(), 1);
  EXPECT_EQ(peer1->get_active_connections_count(), 1);

  // Отключаем второго
  peer1->disconnect_from_peer_by_ip("127.0.0.1");

  std::this_thread::sleep_for(200ms);

  EXPECT_EQ(disconnection_count.load(), 2);
  EXPECT_EQ(peer1->get_active_connections_count(), 0);

  // Останавливаем
  peer1->stop();
  peer2->stop();
  peer3->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 10: Гибридный режим (два узла подключаются друг к другу)
TEST_F(PeerNodeTest, HybridModeTwoWayConnection) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();

  int port1 = getTestPort(9100);
  int port2 = getTestPort(9200);

  std::atomic<int> peer1_connections{0};
  std::atomic<int> peer2_connections{0};

  std::string test_message = "Two-way test";
  std::atomic<int> messages_exchanged{0};

  // Настраиваем peer1
  peer1->set_peer_connected_callback(
      [&](const std::string &) { peer1_connections++; });

  peer1->set_data_callback(
      [&](const std::string &, const std::vector<uint8_t> &data) {
        std::string message(data.begin(), data.end());
        if (message == test_message) {
          messages_exchanged++;
        }
      });

  // Настраиваем peer2
  peer2->set_peer_connected_callback(
      [&](const std::string &) { peer2_connections++; });

  peer2->set_data_callback(
      [&](const std::string &, const std::vector<uint8_t> &data) {
        std::string message(data.begin(), data.end());
        if (message == test_message) {
          messages_exchanged++;
        }
      });

  // Запускаем оба узла в разных потоках
  peer1->start_listening(port1);
  peer2->start_listening(port2);

  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });
  auto thread2 = std::thread([peer2]() { peer2->run_event_loop(); });

  std::this_thread::sleep_for(200ms);

  // Устанавливаем двустороннее соединение
  bool conn1 = peer1->connect_to_peer("127.0.0.1", port2);
  bool conn2 = peer2->connect_to_peer("127.0.0.1", port1);

  ASSERT_TRUE(conn1);
  ASSERT_TRUE(conn2);

  std::this_thread::sleep_for(300ms);

  // Проверяем соединения
  EXPECT_EQ(peer1_connections.load(), 1);
  EXPECT_EQ(peer2_connections.load(), 1);
  EXPECT_EQ(peer1->get_active_connections_count(), 1);
  EXPECT_EQ(peer2->get_active_connections_count(), 1);

  // Отправляем сообщения в обе стороны
  std::vector<uint8_t> data(test_message.begin(), test_message.end());

  peer1->send_to_peer("127.0.0.1", data);
  std::this_thread::sleep_for(100ms);

  peer2->send_to_peer("127.0.0.1", data);
  std::this_thread::sleep_for(100ms);

  // Проверяем обмен сообщениями
  EXPECT_GE(messages_exchanged.load(), 2);

  // Останавливаем
  peer1->stop();
  peer2->stop();

  if (thread1.joinable())
    thread1.join();
  if (thread2.joinable())
    thread2.join();
}

// Тест 11: Большой объем данных
TEST_F(PeerNodeTest, LargeMessageTransfer) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  std::promise<std::vector<uint8_t>> received_promise;
  auto received_future = received_promise.get_future();

  // Генерируем большое сообщение (1MB)
  const size_t message_size = 1024 * 1024; // 1MB
  std::vector<uint8_t> large_message(message_size);

  // Заполняем тестовыми данными
  for (size_t i = 0; i < message_size; ++i) {
    large_message[i] = static_cast<uint8_t>(i % 256);
  }

  // Настраиваем обработчик
  peer2->set_data_callback(
      [&](const std::string &ip, const std::vector<uint8_t> &data) {
        received_promise.set_value(data);
      });

  // Запускаем узлы
  peer1->start_listening(port);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаемся
  bool connected = peer2->connect_to_peer("127.0.0.1", port);
  ASSERT_TRUE(connected);

  std::this_thread::sleep_for(100ms);

  // Отправляем большое сообщение
  peer1->send_to_peer("127.0.0.1", large_message);

  // Ждем получения
  auto status = received_future.wait_for(5s);
  ASSERT_EQ(status, std::future_status::ready);

  std::vector<uint8_t> received = received_future.get();

  // Проверяем целостность данных
  EXPECT_EQ(received.size(), large_message.size());
  EXPECT_EQ(received, large_message);

  // Останавливаем
  peer1->stop();
  peer2->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 12: Стресс-тест множественных подключений
TEST_F(PeerNodeTest, StressTestMultipleConnections) {
  const int NUM_CLIENTS = 10;
  auto server = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  std::atomic<int> total_messages_received{0};

  // Настраиваем сервер
  server->set_data_callback(
      [&](const std::string &, const std::vector<uint8_t> &) {
        total_messages_received++;
      });

  // Запускаем сервер
  server->start_listening(port);
  auto server_thread = std::thread([server]() { server->run_event_loop(); });

  std::this_thread::sleep_for(200ms);

  // Создаем и запускаем клиентов
  std::vector<std::shared_ptr<PeerNode>> clients;
  std::vector<std::thread> client_threads;

  for (int i = 0; i < NUM_CLIENTS; ++i) {
    auto client = PeerNodeFactory::create_tcp_peer();
    clients.push_back(client);

    // Подключаем клиента
    bool connected = client->connect_to_peer("127.0.0.1", port);
    EXPECT_TRUE(connected);

    std::this_thread::sleep_for(10ms); // Небольшая задержка между подключениями
  }

  std::this_thread::sleep_for(
      500ms); // Даем время на установление всех соединений

  // Проверяем количество соединений на сервере
  EXPECT_EQ(server->get_active_connections_count(), NUM_CLIENTS);

  // Отправляем сообщения от всех клиентов
  std::string test_message = "Stress test";
  std::vector<uint8_t> data(test_message.begin(), test_message.end());

  for (auto &client : clients) {
    client->send_to_peer("127.0.0.1", data);
    std::this_thread::sleep_for(5ms);
  }

  std::this_thread::sleep_for(500ms);

  // Проверяем получение сообщений
  EXPECT_GE(total_messages_received.load(), NUM_CLIENTS);

  // Останавливаем всех клиентов
  for (auto &client : clients) {
    client->stop();
  }

  // Останавливаем сервер
  server->stop();

  if (server_thread.joinable())
    server_thread.join();

  // Ждем завершения всех потоков
  for (auto &thread : client_threads) {
    if (thread.joinable())
      thread.join();
  }
}

// Тест 13: Проверка callback'ов подключения/отключения
TEST_F(PeerNodeTest, ConnectionDisconnectionCallbacks) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  std::promise<void> connected_promise;
  std::promise<void> disconnected_promise;

  auto connected_future = connected_promise.get_future();
  auto disconnected_future = disconnected_promise.get_future();

  // Настраиваем callback'и
  peer1->set_peer_connected_callback([&](const std::string &ip) {
    if (ip == "127.0.0.1") {
      connected_promise.set_value();
    }
  });

  peer1->set_peer_disconnected_callback([&](const std::string &ip) {
    if (ip == "127.0.0.1") {
      disconnected_promise.set_value();
    }
  });

  // Запускаем
  peer1->start_listening(port);
  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаем
  bool connected = peer2->connect_to_peer("127.0.0.1", port);
  ASSERT_TRUE(connected);

  // Ждем callback подключения
  auto status = connected_future.wait_for(2s);
  EXPECT_EQ(status, std::future_status::ready);

  std::this_thread::sleep_for(100ms);

  // Отключаем
  peer2->stop();

  // Ждем callback отключения
  status = disconnected_future.wait_for(2s);
  EXPECT_EQ(status, std::future_status::ready);

  // Останавливаем
  peer1->stop();
  if (thread1.joinable())
    thread1.join();
}

// Тест 14: Восстановление после ошибок
TEST_F(PeerNodeTest, RecoveryAfterErrors) {
  auto peer1 = PeerNodeFactory::create_tcp_peer();
  auto peer2 = PeerNodeFactory::create_tcp_peer();

  int port = getTestPort();

  // Запускаем, останавливаем и снова запускаем
  EXPECT_NO_THROW(peer1->start_listening(port));

  auto thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Подключаем
  bool connected = peer2->connect_to_peer("127.0.0.1", port);
  EXPECT_TRUE(connected);

  std::this_thread::sleep_for(100ms);

  // Останавливаем
  peer1->stop();
  if (thread1.joinable())
    thread1.join();

  std::this_thread::sleep_for(200ms);

  // Запускаем снова
  EXPECT_NO_THROW(peer1->start_listening(port));
  thread1 = std::thread([peer1]() { peer1->run_event_loop(); });

  std::this_thread::sleep_for(100ms);

  // Снова подключаем
  connected = peer2->connect_to_peer("127.0.0.1", port);
  EXPECT_TRUE(connected);

  std::this_thread::sleep_for(100ms);

  // Проверяем работу
  EXPECT_EQ(peer1->get_active_connections_count(), 1);

  // Останавливаем
  peer1->stop();
  peer2->stop();
  if (thread1.joinable())
    thread1.join();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
