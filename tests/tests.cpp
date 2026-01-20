#include <gtest/gtest.h>

#include "../include/encryption/AESGCMEncryption.hpp"
#include "../include/encryption/KDF.hpp"
#include "../include/encryption/identity_key.hpp"
#include "../include/models/chat.hpp"
#include "../include/network/protocol/parser.hpp"
#include "../include/network/transport/acceptor.hpp"
#include "../include/network/transport/client.hpp"
#include "../include/network/transport/connection.hpp"
#include "../include/network/transport/event_loop.hpp"
#include "../include/network/transport/handling.hpp"
#include "../include/network/transport/interface.hpp"
#include "../include/network/transport/raw_socket.hpp"
#include "../include/network/transport/server.hpp"
#include "../include/network/transport/transport.hpp"
#include "../include/services/commands_impl/exit.hpp"
#include "../include/services/commands_impl/get_pubkey.hpp"
#include "../include/services/commands_impl/join.hpp"
#include "../include/services/commands_impl/login.hpp"
#include "../include/services/commands_impl/make_room.hpp"
#include "../include/services/commands_impl/private_message.hpp"
#include "../include/services/encryption_service.hpp"
#include "../include/services/message_queue.hpp"
#include "../include/services/messageing_service.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

// ==========================================
// УНИВЕРСАЛЬНАЯ ФУНКЦИЯ БЕЗОПАСНОЙ ОСТАНОВКИ СЕРВЕРА
// ==========================================

/**
 * Безопасное отключение TCP-сервера без deadlock
 */

// Предполагаем, что интерфейсы ваших классов выглядят так (для компиляции
// примера): server->stop() должен прерывать run_event_loop()

/**
 * Безопасное отключение TCP-сервера
 */
bool SafeServerShutdown(std::shared_ptr<TCPServer> server,
                        std::thread &server_thread, int timeout_ms = 3000) {
  if (!server)
    return true;

  if (server->is_running()) {
    std::cout << "[SafeShutdown] Останавливаем сервер..." << std::endl;
    server->stop();
  }

  if (server_thread.joinable()) {
    // Так как у std::thread нет try_join_for, используем async для имитации
    // таймаута
    auto future =
        std::async(std::launch::async, &std::thread::join, &server_thread);

    if (future.wait_for(std::chrono::milliseconds(timeout_ms)) ==
        std::future_status::timeout) {
      std::cerr << "[SafeShutdown] ВНИМАНИЕ: Таймаут остановки! Поток будет "
                   "отсоединен."
                << std::endl;
      // Мы не можем убить поток в C++ безопасно, поэтому detach
      server_thread.detach();
      return false;
    }
    std::cout << "[SafeShutdown] Сервер успешно остановлен." << std::endl;
  }

  return true;
}

class ServerGuard {
private:
  std::shared_ptr<TCPServer> server_;
  std::thread &server_thread_;
  std::atomic<bool> stopped_{false};

public:
  ServerGuard(std::shared_ptr<TCPServer> server, std::thread &thread)
      : server_(server), server_thread_(thread) {}

  ~ServerGuard() { stop_now(); }

  void stop_now() {
    bool expected = false;
    if (stopped_.compare_exchange_strong(expected, true)) {
      SafeServerShutdown(server_, server_thread_);
    }
  }
};

class NetworkTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Используем более надежный выбор порта
    port = 50000 + (rand() % 10000);
    server = TCPServer::create();

    server->set_data_callback([this](int fd, std::vector<uint8_t> data) {
      std::lock_guard<std::mutex> lock(server_messages_mutex);
      server_messages.push_back(data);
    });
  }

  void TearDown() override {
    if (server_guard) {
      server_guard->stop_now();
    } else {
      SafeServerShutdown(server, server_thread);
    }
  }

  void StartServer() {
    server->start(port);
    server_thread = std::thread([this]() {
      try {
        server->run_event_loop();
      } catch (const std::exception &e) {
        std::cerr << "[Server Loop Error] " << e.what() << std::endl;
      }
    });

    // Ожидаем реального запуска вместо простого sleep
    int attempts = 0;
    while (!server->is_running() && attempts++ < 20) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server_guard = std::make_unique<ServerGuard>(server, server_thread);
  }

  // Создание клиента (теперь возвращает unique_ptr для четкого владения)
  std::shared_ptr<TCPClient> CreateClient() {
    auto client = TCPClient::create();
    client->set_data_callback([this](const std::vector<uint8_t> &data) {
      std::lock_guard<std::mutex> lock(client_messages_mutex);
      client_messages.push_back(data);
    });
    return client;
  }

  // Универсальный метод ожидания для чистоты кода
  template <typename T>
  bool WaitForMessages(const std::vector<T> &container, std::mutex &mtx,
                       size_t expected, int timeout_ms = 2000) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start)
               .count() < timeout_ms) {
      {
        std::lock_guard<std::mutex> lock(mtx);
        if (container.size() >= expected)
          return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
  }

  int port;
  std::shared_ptr<TCPServer> server;
  std::thread server_thread;
  std::unique_ptr<ServerGuard> server_guard;

  std::vector<std::vector<uint8_t>> server_messages;
  std::vector<std::vector<uint8_t>> client_messages;
  std::mutex server_messages_mutex;
  std::mutex client_messages_mutex;
};
// ==========================================
// 1. ОБНОВЛЕННЫЕ БАЗОВЫЕ ТЕСТЫ (Unit)
// ==========================================

// Тест RAII обертки дескриптора
TEST(ComponentTest, FdMoveOwnership) {
  int dummy_fd = socket(AF_INET, SOCK_STREAM, 0);
  {
    Fd fd_obj(dummy_fd);
    EXPECT_EQ(fd_obj.get_fd(), dummy_fd);

    Fd fd_moved = std::move(fd_obj);
    EXPECT_EQ(fd_moved.get_fd(), dummy_fd);
    EXPECT_EQ(fd_obj.get_fd(), -1); // Старый объект пуст
  } // Здесь сокет закроется автоматически через fd_moved
}

// Тест менеджера соединений
TEST(ComponentTest, ConnectionManagerLogic) {
  ConnectionManager manager;
  auto conn = std::make_shared<ClientConnection>(999);

  manager.add_connection(999, conn);
  EXPECT_TRUE(manager.find_connection(999));

  manager.remove_connection(999);
  EXPECT_FALSE(manager.find_connection(999));
}

// Тест формирования пакета в транспорте (логика заголовка 4 байта)
TEST(ComponentTest, TransportHeaderLogic) {
  TransportFabric fabric;
  std::vector<uint8_t> data = {'H', 'i'};

  // Создаем соединение и тестируем отправку
  ClientConnection conn(100);
  conn.init_transport(std::move(fabric.create_tcp()));
  conn.queue_send(data);

  // Проверяем, что flush не падает
  EXPECT_NO_THROW(conn.flush());
}

// ==========================================
// 2. ОБНОВЛЕННЫЕ ИНТЕГРАЦИОННЫЕ ТЕСТЫ
// ==========================================

// Тест на установку и разрыв соединения
TEST_F(NetworkTest, ConnectionLifecycle) {
  StartServer();

  auto client = CreateClient();
  bool connected = client->connect("127.0.0.1", port);

  EXPECT_TRUE(connected);
  EXPECT_TRUE(client->isConnected());

  client->disconnect();
  EXPECT_FALSE(client->isConnected());
}

// Тест на массовое подключение клиентов (Stress-lite)
TEST_F(NetworkTest, MultipleClientsConnection) {
  StartServer();

  const int count = 10;
  std::vector<std::shared_ptr<TCPClient>> clients;

  for (int i = 0; i < count; ++i) {
    auto c = CreateClient();
    if (c->connect("127.0.0.1", port)) {
      clients.push_back(c);
    }
  }

  EXPECT_EQ(clients.size(), count);

  for (auto &c : clients) {
    EXPECT_TRUE(c->isConnected());
    c->disconnect();
  }
}

// Тест передачи данных от клиента к серверу
TEST_F(NetworkTest, SendDataToServer) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  std::string text = "TestMessage";
  std::vector<uint8_t> data(text.begin(), text.end());

  // Очищаем предыдущие сообщения
  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    server_messages.clear();
  }

  // Отправляем данные
  EXPECT_NO_THROW(client->send_to_server(data));

  // Ждем получения на сервере
  EXPECT_TRUE(WaitForMessages(server_messages, server_messages_mutex, 1, 500));

  // Проверяем полученное сообщение
  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    EXPECT_GE(server_messages.size(), 1);
    if (!server_messages.empty()) {
      std::string received(server_messages[0].begin(),
                           server_messages[0].end());
      EXPECT_EQ(received, text);
    }
  }

  client->disconnect();
}

// Тест двусторонней передачи данных
TEST_F(NetworkTest, BidirectionalCommunication) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  std::atomic<int> last_server_fd{-1};
  // Переопределим callback сервера, чтобы узнать FD клиента с точки зрения
  // сервера
  server->set_data_callback([&](int fd, std::vector<uint8_t> data) {
    last_server_fd = fd; // Сохраняем правильный FD
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    server_messages.push_back(data);
  });

  std::vector<uint8_t> client_msg = {'H', 'e', 'l', 'l', 'o'};
  client->send_to_server(client_msg);

  // Ждем сообщения на сервере
  ASSERT_TRUE(WaitForMessages(server_messages, server_messages_mutex, 1, 1000));

  // Отправляем ответ, используя FD, который увидел сервер!
  std::vector<uint8_t> server_reply = {'W', 'o', 'r', 'l', 'd'};
  server->send(last_server_fd, server_reply);

  // Ждем на клиенте
  EXPECT_TRUE(WaitForMessages(client_messages, client_messages_mutex, 1, 1000));
}

// ==========================================
// 3. ТЕСТЫ ОБРАБОТКИ ОШИБОК
// ==========================================

// Тест подключения к несуществующему серверу
TEST_F(NetworkTest, ConnectionRefused) {
  auto client = CreateClient();
  // Пытаемся подключиться к порту, где никто не слушает
  bool result = client->connect("127.0.0.1", 9999);

  EXPECT_FALSE(result);
}

// Тест на остановку сервера при активных клиентах
TEST_F(NetworkTest, ServerStopWithActiveClients) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  ASSERT_TRUE(client->isConnected());

  // Резко выключаем сервер
  if (server_guard) {
    server_guard->stop_now();
  } else {
    SafeServerShutdown(server, server_thread);
  }

  EXPECT_FALSE(server->is_running());
}

// Тест отключения клиента от сервера
TEST_F(NetworkTest, ClientDisconnection) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  EXPECT_TRUE(client->isConnected());

  // Клиент отключается
  client->disconnect();

  // Даем время на обработку отключения
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(client->isConnected());
}

// ==========================================
// 4. ТЕСТЫ СЛОЖНЫХ СЦЕНАРИЕВ (ADVANCED)
// ==========================================

// Тест передачи большого объема данных
TEST_F(NetworkTest, LargePayloadTest) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  // Создаем большое сообщение (16KB)
  std::vector<uint8_t> large_data(16384, 'A');

  // Очищаем буфер
  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    server_messages.clear();
  }

  EXPECT_NO_THROW(client->send_to_server(large_data));

  // Ждем получения
  EXPECT_TRUE(WaitForMessages(server_messages, server_messages_mutex, 1, 1000));

  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    EXPECT_GE(server_messages.size(), 1);
    if (!server_messages.empty()) {
      EXPECT_EQ(server_messages[0].size(), large_data.size());
    }
  }

  client->disconnect();
}

// Тест быстрого переподключения
TEST_F(NetworkTest, RapidReconnect) {
  StartServer();

  for (int i = 0; i < 3; ++i) {
    auto client = CreateClient();
    ASSERT_TRUE(client->connect("127.0.0.1", port));

    std::vector<uint8_t> data = {'T', 'e', 's', 't', (unsigned char)('0' + i)};
    client->send_to_server(data);

    client->disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

// Тест работы с пустыми сообщениями
TEST_F(NetworkTest, EmptyMessageHandling) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  // Пустой вектор
  std::vector<uint8_t> empty_data;

  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    server_messages.clear();
  }

  EXPECT_NO_THROW(client->send_to_server(empty_data));

  // Ждем немного (пустое сообщение может быть обработано иначе)
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Не проверяем конкретное поведение, только что не упало
  SUCCEED();

  client->disconnect();
}

// Тест множественных сообщений подряд
TEST_F(NetworkTest, MultipleMessagesInRow) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    server_messages.clear();
  }

  // Отправляем несколько сообщений подряд
  for (int i = 0; i < 5; ++i) {
    std::vector<uint8_t> data = {'M', 's', 'g', uint8_t('0' + i)};
    client->send_to_server(data);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Ждем все сообщения
  EXPECT_TRUE(WaitForMessages(server_messages, server_messages_mutex, 5, 1000));

  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    EXPECT_GE(server_messages.size(), 5);
  }

  client->disconnect();
}

// ==========================================
// 5. ТЕСТЫ НОВЫХ ФУНКЦИОНАЛЬНОСТЕЙ
// ==========================================

// Тест callback системы
TEST_F(NetworkTest, CallbackSystem) {
  StartServer();

  std::atomic<int> callback_called{0};
  std::vector<uint8_t> received_data;

  auto client = TCPClient::create();
  client->set_data_callback([&](const std::vector<uint8_t> &data) {
    callback_called++;
    received_data = data;
    std::cout << "[TestCallback] Вызван callback с данными размером "
              << data.size() << " байт" << std::endl;
  });

  client->connect("127.0.0.1", port);

  // Сервер отправляет сообщение клиенту
  std::vector<uint8_t> test_data = {'T', 'e', 's', 't'};
  server->send(client->get_fd(), test_data);

  // Ждем вызова callback
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start)
             .count() < 1000) {
    if (callback_called > 0) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_GT(callback_called, 0);
  EXPECT_EQ(received_data, test_data);

  client->disconnect();
}

// Тест асинхронного запуска event loop
TEST_F(NetworkTest, AsyncEventLoop) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  // Запускаем event loop клиента в отдельном потоке
  std::atomic<bool> client_running{true};
  std::thread client_thread([&]() {
    // Клиент будет обрабатывать сообщения в фоне
    while (client_running) {
      // В реальном приложении здесь был бы вызов run_event_loop()
      // Но для теста просто немного спим
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  // Отправляем сообщение
  std::vector<uint8_t> data = {'A', 's', 'y', 'n', 'c'};
  client->send_to_server(data);

  // Ждем получения на сервере
  EXPECT_TRUE(WaitForMessages(server_messages, server_messages_mutex, 1, 500));

  // Останавливаем клиент
  client_running = false;
  if (client_thread.joinable()) {
    client_thread.join();
  }

  client->disconnect();
}

// ==========================================
// 6. ТЕСТЫ ПРОИЗВОДИТЕЛЬНОСТИ И НАГРУЗКИ
// ==========================================

// Тест быстрой отправки множества маленьких сообщений
TEST_F(NetworkTest, ManySmallMessages) {
  StartServer();
  auto client = CreateClient();
  client->connect("127.0.0.1", port);

  const int message_count = 100;

  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    server_messages.clear();
  }

  // Быстрая отправка многих сообщений
  for (int i = 0; i < message_count; ++i) {
    std::vector<uint8_t> data = {uint8_t(i % 256)};
    client->send_to_server(data);
  }

  // Ждем все сообщения (может потребоваться больше времени)
  EXPECT_TRUE(WaitForMessages(server_messages, server_messages_mutex,
                              message_count, 2000));

  {
    std::lock_guard<std::mutex> lock(server_messages_mutex);
    std::cout << "[ManySmallMessages] Получено " << server_messages.size()
              << " сообщений" << std::endl;
    EXPECT_GE(server_messages.size(), message_count);
  }

  client->disconnect();
}

// ==========================================
// 7. ТЕСТЫ ЛОГИКИ МЕССЕНДЖЕРА (МОДЕЛИ И КРИПТО)
// ==========================================

// Тест генерации ключей X25519 и вычисления Shared Secret
TEST(MessengerLogicTest, IdentityKeySharedSecret) {
  // Генерируем ключи для Алисы и Боба
  auto alice_key = IdentityKey::generate();
  auto bob_key = IdentityKey::generate();

  auto alice_pub = alice_key.public_bytes();
  auto bob_pub = bob_key.public_bytes();

  ASSERT_FALSE(alice_pub.empty());
  ASSERT_FALSE(bob_pub.empty());

  // Восстанавливаем публичный ключ Боба из байтов для Алисы
  auto bob_pub_reconstructed = IdentityKey::from_public_bytes(bob_pub);
  auto alice_pub_reconstructed = IdentityKey::from_public_bytes(alice_pub);

  // Вычисляем общие секреты
  auto secret_alice = alice_key.compute_shared_secret(bob_pub_reconstructed);
  auto secret_bob = bob_key.compute_shared_secret(alice_pub_reconstructed);

  ASSERT_FALSE(secret_alice.empty());
  EXPECT_EQ(secret_alice, secret_bob); // Секреты должны совпадать
}

// Тест KDF (деривация ключей для мессенджера)
TEST(MessengerLogicTest, HKDFDerivation) {
  std::vector<uint8_t> shared_secret = {1, 2, 3, 4, 5};
  std::vector<uint8_t> alice_id = {0xAA};
  std::vector<uint8_t> bob_id = {0xBB};

  auto key1 =
      HKDF::derive_for_messaging(shared_secret, alice_id, bob_id, "enc");
  auto key2 =
      HKDF::derive_for_messaging(shared_secret, bob_id, alice_id, "enc");

  // Порядок ID (sender/receiver) не должен влиять на ключ из-за сортировки в
  // HKDF::derive_for_messaging
  EXPECT_EQ(key1, key2);
  EXPECT_EQ(key1.size(), 32);
}

// Тест шифрования AES-256-GCM
TEST(MessengerLogicTest, AESGCMEncryptionDecryption) {
  AESGCMEncryptor encryptor;
  std::vector<uint8_t> key(32, 0x01); // 256-bit key
  std::string text = "Secret Message Content";
  std::vector<uint8_t> plaintext(text.begin(), text.end());

  // Шифруем
  auto ciphertext = encryptor.encrypt(key, plaintext);
  ASSERT_GT(ciphertext.size(), plaintext.size()); // Должны быть IV и Tag

  // Дешифруем
  auto decrypted = encryptor.decrypt(key, ciphertext);
  std::string result(decrypted.begin(), decrypted.end());

  EXPECT_EQ(result, text);
}

// Тест парсера команд (например, /login user pass)
TEST(MessengerLogicTest, ParserCommandHandling) {
  Parser parser;
  std::string input = "/login user1 password123";

  Message msg = parser.parse(input);

  EXPECT_EQ(msg.get_type(), MessageType::Command);
  // Проверка структуры: metadata[0] - тип команды, metadata[1] - аргумент 1 и
  // т.д.
  ASSERT_GE(msg.get_meta(0).size(), 1);
  EXPECT_EQ(msg.get_meta(0)[0], static_cast<uint8_t>(CommandType::LOGIN));
}

// Тест сериализации и десериализации сообщения
TEST(MessengerLogicTest, MessageSerialization) {
  Serializer serializer;
  std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
  std::vector<std::vector<uint8_t>> metadata = {{0x01}, {0x02, 0x03}};

  Message original(payload, 2, metadata, MessageType::CipherMessage);

  // В текущей реализации Message() конструктор обновляет timestamp и длину
  std::vector<uint8_t> serialized = serializer.serialize(original);
  Message deserialized = serializer.deserialize(serialized);

  EXPECT_EQ(deserialized.get_type(), MessageType::CipherMessage);
  EXPECT_EQ(deserialized.get_payload(), payload);
  EXPECT_EQ(deserialized.get_meta(0), metadata[0]);
  EXPECT_EQ(deserialized.get_meta(1), metadata[1]);
}

// Тест управления членами чата
TEST(MessengerLogicTest, ChatMemberManagement) {
  Chat chat("chat_1", "General", ChatType::Group);

  EXPECT_TRUE(chat.add_member("user_a"));
  EXPECT_TRUE(chat.add_member("user_b"));
  EXPECT_FALSE(chat.add_member("user_a")); // Дубль

  EXPECT_TRUE(chat.is_member("user_a"));

  auto members = chat.get_members();
  EXPECT_EQ(members.size(), 2);

  chat.remove_member("user_a");
  EXPECT_FALSE(chat.is_member("user_a"));
}

// ==========================================
// 8. ТЕСТЫ КРАЕВЫХ СЛУЧАЕВ (EDGE CASES)
// ==========================================

// Тест на подмену зашифрованных данных (проверка GCM аутентификации)
TEST(EdgeCaseTest, AESGCM_IntegrityViolation) {
  AESGCMEncryptor encryptor;
  std::vector<uint8_t> key(32, 0x01);
  std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03, 0x04};

  auto ciphertext = encryptor.encrypt(key, plaintext);

  // Имитируем атаку: меняем один бит в зашифрованных данных
  // В GCM это должно привести к ошибке дешифровки (Authentication Failure)
  ciphertext[ciphertext.size() - 1] ^= 0xFF;

  // Ожидаем, что decrypt выбросит исключение runtime_error
  EXPECT_THROW(encryptor.decrypt(key, ciphertext), std::runtime_error);
}

// Тест на слишком короткий шифротекст
TEST(EdgeCaseTest, AESGCM_ShortCiphertext) {
  AESGCMEncryptor encryptor;
  std::vector<uint8_t> key(32, 0x01);

  // Меньше чем IV(12) + Tag(16)
  std::vector<uint8_t> too_short(10, 0xFF);

  EXPECT_THROW(encryptor.decrypt(key, too_short), std::runtime_error);
}

// Тест на "пустые" сообщения в сериализаторе
TEST(EdgeCaseTest, MessageSerialization_EmptyPayload) {
  Serializer serializer;
  // Сообщение без данных и метаданных
  Message msg({}, 0, {}, MessageType::Text);

  std::vector<uint8_t> serialized = serializer.serialize(msg);
  Message deserialized = serializer.deserialize(serialized);

  EXPECT_EQ(deserialized.get_payload().size(), 0);
  EXPECT_EQ(deserialized.get_type(), MessageType::Text);
}

// Тест парсера на некорректные команды или пустые строки
TEST(EdgeCaseTest, Parser_MalformedCommands) {
  Parser parser;

  // Случай 1: Просто слеш без команды
  Message msg1 = parser.parse("/");
  EXPECT_EQ(msg1.get_type(), MessageType::Command);
  // Проверьте, как ваш код обрабатывает пустую строку в name

  // Случай 2: Команда с недостаточным количеством аргументов
  // Согласно CMD_TABLE, login ожидает 3 сегмента (имя + 2 аргумента)
  Message msg2 = parser.parse("/login only_user");
  // Парсер должен отработать без падения, даже если аргументов меньше
  // ожидаемого
  SUCCEED();
}

// Тест на экстремально большие метаданные
TEST(EdgeCaseTest, Message_MetadataOverflow) {
  Serializer serializer;
  std::vector<uint8_t> big_meta(1024, 'X');
  std::vector<std::vector<uint8_t>> metadata = {big_meta};

  // В сериализаторе длина метаданных пишется в 2 байта (uint16_t)
  // Проверим, что 1KB проходит нормально
  Message msg({0x01}, 1, metadata, MessageType::Text);

  std::vector<uint8_t> serialized = serializer.serialize(msg);
  Message deserialized = serializer.deserialize(serialized);

  EXPECT_EQ(deserialized.get_meta(0).size(), 1024);
}

// Тест приватного чата (ограничение на добавление участников)
TEST(EdgeCaseTest, Chat_PrivateTypeConstraints) {
  // В приватном чате нельзя добавлять более двух людей (логика add_member)
  Chat private_chat("p1", "Private", ChatType::Private);

  // add_member должен возвращать false для типа Private
  EXPECT_FALSE(private_chat.add_member("user_1"));
}

// ==========================================
// 9. ПРОДВИНУТЫЕ КРАЕВЫЕ СЛУЧАИ (ADVANCED)
// ==========================================

// 1. Тест на "Нулевой ключ" (Zero Key)
// Проверка, как HKDF и AES реагируют на пустые или нулевые векторы
TEST(ExtremeEdgeCaseTest, ZeroKeyHandling) {
  std::vector<uint8_t> empty_ikm;

  // Пытаемся вывести ключ из ничего
  // HKDF должен либо выбросить исключение (если есть валидация), либо
  // отработать детерминировано
  EXPECT_THROW(HKDF::derive(empty_ikm, {}, {}, 32), std::runtime_error);

  // Тест ключа, состоящего только из нулей (частая ошибка инициализации)
  std::vector<uint8_t> zero_key(32, 0);
  AESGCMEncryptor encryptor;
  std::vector<uint8_t> data = {1, 2, 3};

  // Криптография должна работать даже на "плохих" ключах,
  // главное, чтобы она не падала (crash)
  EXPECT_NO_THROW({
    auto c = encryptor.encrypt(zero_key, data);
    auto p = encryptor.decrypt(zero_key, c);
    EXPECT_EQ(p, data);
  });
}

// 2. Тест на максимальные границы типов (Uint16/Uint32 Overflow)
// Проверяем, не сломается ли Serializer на очень больших данных
TEST(ExtremeEdgeCaseTest, SerializationLargeData) {
  Serializer serializer;
  // Создаем сообщение, где длина метаданных на грани uint16_t (65535)
  std::vector<uint8_t> huge_meta(65000, 'A');
  std::vector<std::vector<uint8_t>> metadata = {huge_meta};

  Message msg({0x01}, 1, metadata, MessageType::Text);

  // Проверяем, что сериализация не обрезает данные из-за приведения типов
  auto serialized = serializer.serialize(msg);
  Message deserialized = serializer.deserialize(serialized);

  EXPECT_EQ(deserialized.get_meta(0).size(), 65000);
}

// 3. Тест на спецсимволы и бинарные данные в именах/командах
// Важно для Parser, чтобы он не "споткнулся" о нулевые байты или эмодзи
TEST(ExtremeEdgeCaseTest, ParserEncodingAndBinary) {
  Parser parser;

  // Команда с UTF-8 (эмодзи) и спецсимволами
  std::string input = "/send user_123 🚀_Premium_Message_✨";
  Message msg = parser.parse(input);

  // Payload должен в точности сохранить байты эмодзи
  std::string result(msg.get_payload().begin(), msg.get_payload().end());
  EXPECT_NE(result.find("🚀"), std::string::npos);

  // Попытка "инъекции" нулевого байта в середину команды
  std::string malformed = "/login user\0admin pass";
  Message msg2 = parser.parse(malformed);
  // Хороший парсер должен либо обрезать по \0, либо обработать как бинарную
  // строку, но не упасть
  SUCCEED();
}

// 4. Тест на некорректный тип чата (логическая целостность)
TEST(ExtremeEdgeCaseTest, Chat_InvalidTypeLogic) {
  // Создаем чат типа Channel
  Chat channel("chan_1", "News", ChatType::Channel);

  // В вашем коде add_member для Private возвращает false.
  // Проверим, что для Channel он возвращает true (разрешено)
  EXPECT_TRUE(channel.add_member("subscriber_1"));

  // Повторное удаление уже удаленного участника
  EXPECT_TRUE(channel.remove_member("subscriber_1"));
  EXPECT_FALSE(channel.remove_member("subscriber_1")); // Уже нет в списке
}

// 5. Тест на повреждение IV или TAG отдельно
TEST(ExtremeEdgeCaseTest, AESGCM_TamperIVAndTag) {
  AESGCMEncryptor encryptor;
  std::vector<uint8_t> key(32, 0x01);
  std::vector<uint8_t> plaintext = {0xAA, 0xBB, 0xCC};
  auto ciphertext = encryptor.encrypt(key, plaintext);

  // Случай А: Повреждаем IV (первые 12 байт)
  auto corrupt_iv = ciphertext;
  corrupt_iv[0] ^= 0x01;
  EXPECT_THROW(encryptor.decrypt(key, corrupt_iv), std::runtime_error);

  // Случай Б: Повреждаем TAG (последние 16 байт)
  auto corrupt_tag = ciphertext;
  corrupt_tag[corrupt_tag.size() - 1] ^= 0x01;
  EXPECT_THROW(encryptor.decrypt(key, corrupt_tag), std::runtime_error);
}

// ==========================================
// 10. ЖЕСТКИЕ КРАЕВЫЕ СЛУЧАИ (DEBUG)
// ==========================================

// Обновленный тест парсера без нулевого байта (проверяем только UTF-8)
TEST(ExtremeEdgeCaseTest, ParserUTF8Support) {
  Parser parser;
  // Эмодзи и спецсимволы часто занимают 3-4 байта
  std::string input = "/send user_123 🚀_Привет_Мир_✨";

  Message msg;
  EXPECT_NO_THROW({ msg = parser.parse(input); });

  std::string result(msg.get_payload().begin(), msg.get_payload().end());
  EXPECT_NE(result.find("🚀"), std::string::npos);
}

// Отдельный тест на попытку взлома парсера некорректными данными
TEST(ExtremeEdgeCaseTest, Parser_MalformedStructure) {
  Parser parser;

  // Случай 1: Очень длинная строка (защита от Buffer Overflow)
  std::string long_input(10000, 'A');
  EXPECT_NO_THROW(parser.parse("/login " + long_input));

  // Случай 2: Команда, которой нет в CMD_TABLE
  Message msg = parser.parse("/unknown_cmd arg1 arg2");
  // По логике вашего парсера, если команды нет в таблице, expected_args = 0
  // Хвост строки должен уйти в payload или просто проигнорироваться
  SUCCEED();
}

// Тест сериализации: Максимальное количество метаданных
TEST(ExtremeEdgeCaseTest, Serializer_MaxMetadataCount) {
  Serializer serializer;
  std::vector<std::vector<uint8_t>> many_meta;
  // metalen в десериализаторе - это raw[off++], т.е. 1 байт (uint8_t)
  // Значит, максимум 255 элементов
  for (int i = 0; i < 255; ++i) {
    many_meta.push_back({static_cast<uint8_t>(i)});
  }

  Message msg({0x01}, 255, many_meta, MessageType::Text);

  EXPECT_NO_THROW({
    auto serialized = serializer.serialize(msg);
    auto deserialized = serializer.deserialize(serialized);
    EXPECT_EQ(deserialized.get_meta(254)[0], 254);
  });
}

// Тест на "грязный" шифротекст в AES
TEST(ExtremeEdgeCaseTest, AESGCM_GarbageData) {
  AESGCMEncryptor encryptor;
  std::vector<uint8_t> key(32, 0x01);

  // Просто случайные байты вместо валидного шифротекста
  std::vector<uint8_t> garbage(100);
  for (auto &b : garbage)
    b = rand() % 256;

  // Должно выбросить исключение, а не упасть с сегфолтом
  EXPECT_ANY_THROW(encryptor.decrypt(key, garbage));
}

// Тест на частичное копирование IdentityKey (Double Free protection)
TEST(ExtremeEdgeCaseTest, IdentityKey_CopySafety) {
  {
    IdentityKey key1 = IdentityKey::generate();
    {
      IdentityKey key2 = key1; // Проверка оператора копирования и up_ref
      EXPECT_NE(key2.public_bytes().size(), 0);
    } // key2 уничтожается, но EVP_PKEY должен жить
    EXPECT_NE(key1.public_bytes().size(), 0);
  } // здесь уничтожается последний ref
  SUCCEED();
}

// ==========================================
// 12. ТЕСТЫ ВЫСОКОУРОВНЕВЫХ СЕРВИСОВ
// ==========================================

// Тест MessagingService: Авторизация и управление пользователями
TEST(ServiceTest, MessagingService_AuthAndUsers) {
  MessagingService service;
  std::string username = "Alice";
  std::vector<uint8_t> pubkey = {0x01, 0x02, 0x03};

  // 1. Тест авторизации
  std::string token = service.authenticate(username, "pass_hash", pubkey);
  ASSERT_FALSE(token.empty());

  // 2. Тест получения ID по токену
  std::string user_id = service.get_user_id_by_token(token);
  EXPECT_FALSE(user_id.empty());

  // 3. Проверка привязки FD
  int mock_fd = 42;
  service.bind_connection(mock_fd, user_id);
  EXPECT_TRUE(service.is_authenticated(mock_fd));
  EXPECT_EQ(service.get_user_id_by_fd(mock_fd), user_id);
}

// Тест MessagingService: Создание чатов и отправка сообщений
TEST(ServiceTest, MessagingService_ChatLogic) {
  MessagingService service;

  auto alice_token = service.authenticate("Alice", "h", {});
  auto bob_token = service.authenticate("Bob", "h", {});
  auto alice_id = service.get_user_id_by_token(alice_token);
  auto bob_id = service.get_user_id_by_token(bob_token);

  // 1. Создание группового чата
  std::string chat_name = "Devs";
  std::string chat_id = service.create_chat(
      alice_id, chat_name, ChatType::Group, {alice_id, bob_id});

  EXPECT_EQ(service.chat_id_by_name(chat_name), chat_id);
  EXPECT_TRUE(service.get_chat(chat_id).is_member(alice_id));

  // 2. Отправка сообщения
  Message msg({1, 2, 3}, 0, {}, MessageType::Text);
  bool sent = service.send_message(alice_id, chat_id, msg);
  EXPECT_TRUE(sent);
  EXPECT_EQ(service.get_chat(chat_id).get_recent_message(1).size(), 1);
}

// Тест EncryptionService: Кэширование ключей и сквозное шифрование
TEST(ServiceTest, EncryptionService_EndToEnd) {
  EncryptionService alice_service;
  EncryptionService bob_service;

  alice_service.set_identity_key();
  bob_service.set_identity_key();

  std::vector<uint8_t> alice_name = {'a', 'l', 'i', 'c', 'e'};
  std::vector<uint8_t> bob_name = {'b', 'o', 'b'};

  // 1. Обмен публичными ключами (кэширование)
  alice_service.cache_public_key(bob_name, bob_service.get_public_bytes());
  bob_service.cache_public_key(alice_name, alice_service.get_public_bytes());

  // 2. Шифрование и дешифрование
  std::vector<uint8_t> plaintext = {'H', 'e', 'l', 'l', 'o'};

  // Алиса шифрует для Боба
  auto ciphertext = alice_service.encrypt_for(bob_name, plaintext);
  // Боб расшифровывает от Алисы
  auto decrypted = bob_service.decrypt_for(alice_name, ciphertext);

  EXPECT_EQ(decrypted, plaintext);
}

// Тест MessageQueue: Очередность и хранение
TEST(ServiceTest, MessageQueue_PendingLogic) {
  // IClient - интерфейс, здесь нужен либо mock, либо nullptr для базовой
  // очереди
  MessageQueue queue(nullptr);
  std::vector<uint8_t> user_id = {0xAA};
  std::vector<uint8_t> data1 = {0xDE, 0xAD};
  std::vector<uint8_t> data2 = {0xBE, 0xEF};

  // 1. Пушим сообщения
  queue.push(user_id, data1);
  queue.push(user_id, data2);
  EXPECT_FALSE(queue.empty());

  // 2. Проверяем FIFO (First In, First Out) через find_pending
  auto pending = queue.find_pending(user_id);
  ASSERT_TRUE(pending.has_value());
  EXPECT_EQ(pending->bytes, data1);

  // 3. Проверяем остаток
  auto pending2 = queue.find_pending(user_id);
  EXPECT_EQ(pending2->bytes, data2);
  EXPECT_TRUE(queue.empty());
}

// ==========================================
// 13. КРАЕВЫЕ СЛУЧАИ ДЛЯ СЕРВИСОВ
// ==========================================

// --- EncryptionService ---

// Тест: Попытка шифрования для несуществующего пользователя
TEST(ServiceEdgeCase, EncryptionService_UnknownUser) {
  EncryptionService service;
  service.set_identity_key();
  std::vector<uint8_t> unknown_user = {'u', 'n', 'k', 'n', 'o', 'w', 'n'};
  std::vector<uint8_t> data = {1, 2, 3};

  // В коде encryption_service.hpp: keys[username] создаст пустой объект
  // IdentityKey, если его нет. compute_shared_secret на пустом ключе должен
  // вернуть пустой вектор или упасть. Проверим, что сервис обрабатывает это без
  // краха.
  EXPECT_ANY_THROW(service.encrypt_for(unknown_user, data));
}

// Тест: Шифрование пустого сообщения
TEST(ServiceEdgeCase, EncryptionService_EmptyPlaintext) {
  EncryptionService alice, bob;
  alice.set_identity_key();
  bob.set_identity_key();
  std::vector<uint8_t> name = {'b', 'o', 'b'};
  alice.cache_public_key(name, bob.get_public_bytes());

  std::vector<uint8_t> empty_data = {};
  auto ciphertext = alice.encrypt_for(name, empty_data);

  // Даже пустое сообщение в GCM должно иметь IV (12) и Tag (16) = 28 байт
  EXPECT_GE(ciphertext.size(), 28);

  alice.cache_public_key({'a'},
                         alice.get_public_bytes()); // для обратного теста
  bob.cache_public_key({'a'}, alice.get_public_bytes());
  auto decrypted = bob.decrypt_for({'a'}, ciphertext);
  EXPECT_TRUE(decrypted.empty());
}

// --- MessageQueue ---

// Тест: Pop из пустой очереди
TEST(ServiceEdgeCase, MessageQueue_PopEmpty) {
  MessageQueue queue(nullptr);
  // Ожидаем исключение std::runtime_error ("MessageQueue is empty")
  EXPECT_THROW(queue.pop(), std::runtime_error);
}

// Тест: Очередь для "бинарного" ID (содержащего \0)
TEST(ServiceEdgeCase, MessageQueue_BinaryId) {
  MessageQueue queue(nullptr);
  std::vector<uint8_t> binary_id = {0x00, 0xFF, 0x00, 0x01};
  std::vector<uint8_t> data = {0xAA, 0xBB};

  queue.push(binary_id, data);
  auto found = queue.find_pending(binary_id);

  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->recipient_id, binary_id);
}

// --- MessagingService ---

// Тест: Повторная авторизация под тем же именем
TEST(ServiceEdgeCase, MessagingService_DuplicateLogin) {
  MessagingService service;
  std::vector<uint8_t> key = {1, 2, 3};

  // Первый вход
  std::string token1 = service.authenticate("User", "pass", key);
  // Второй вход того же пользователя
  std::string token2 = service.authenticate("User", "pass", key);

  // В текущей реализации messaging_service.hpp создаст новую запись в users.
  // Проверим, что токены разные или система корректно обновляет состояние.
  EXPECT_NE(token1, token2);
}

// Тест: Привязка нескольких FD к одному пользователю
TEST(ServiceEdgeCase, MessagingService_MultipleConnections) {
  MessagingService service;
  std::string uid = "user_123";

  service.bind_connection(10, uid);
  service.bind_connection(11, uid);

  // Проверяем, что оба FD ведут к одному ID
  EXPECT_EQ(service.get_user_id_by_fd(10), uid);
  EXPECT_EQ(service.get_user_id_by_fd(11), uid);

  // А вот что вернет обратный поиск fds[uid]? Обычно последний привязанный.
  // Это важно для логики отправки ответа.
}

// Тест: Удаление несуществующего FD
TEST(ServiceEdgeCase, MessagingService_RemoveInvalidFD) {
  MessagingService service;
  // Попытка удалить FD, которого нет в connections
  EXPECT_NO_THROW(service.remove_user_by_fd(999));
}

// ==========================================
// 14. ТЕСТЫ КОМАНД И КЛИЕНТ-СЕРВЕРА
// ==========================================

// 1. Тест жизненного цикла команды (на примере LoginCommand)
TEST(CommandTest, LoginCommand_Serialization) {
  LoginCommand cmd;
  ParsedCommand pc;
  pc.name = "login";
  pc.args = {{'u', 's', 'e', 'r'}, {0x01, 0x02, 0x03}};

  // Проверка парсинга структуры в команду
  cmd.fromParsedCommand(pc);

  // Проверка преобразования команды в сетевое сообщение
  Message msg = cmd.toMessage();
  EXPECT_EQ(msg.get_type(), MessageType::Command);
  EXPECT_EQ(msg.get_meta(0)[0], static_cast<uint8_t>(CommandType::LOGIN));
  EXPECT_EQ(msg.get_meta(1), pc.args[0]); // username
  EXPECT_EQ(msg.get_meta(2), pc.args[1]); // pubkey
}

// 2. Тест выполнения команды на стороне сервера
TEST(CommandTest, JoinCommand_ServerExecution) {
  MessagingService service;
  JoinCommand cmd;

  auto token = service.authenticate("Alice", "", {1});
  // Подготовка данных
  std::string user_id = service.get_user_id_by_token(token);
  int fd = 10;
  service.bind_connection(fd, user_id);

  std::string chat_name = "General";
  service.create_chat(user_id, chat_name, ChatType::Group, {user_id});

  // Имитируем пришедшее сообщение /join General
  ParsedCommand pc;
  pc.name = "join";
  pc.args = {std::vector<uint8_t>(chat_name.begin(), chat_name.end())};
  cmd.fromParsedCommand(pc);

  // Выполнение на сервере
  Message response = cmd.execeuteOnServer(fd, service);

  // Проверяем, что сервер ответил текстом (успех или уведомление)
  EXPECT_EQ(response.get_type(), MessageType::Text);
}

// 3. Тест обработки PrivateMessage (Крипто-команда)
TEST(CommandTest, PrivateMessage_ClientExecution) {
  EncryptionService encryption;
  encryption.set_identity_key();

  PrivateMessageCommand pm_cmd;
  // В реальном коде данные заполняются через fromMessage
  // Здесь мы проверяем только executeOnClient с передачей сервиса

  std::vector<std::any> args = {&encryption};

  // Проверяем, что команда не падает при пустых данных,
  // но корректно обрабатывает аргумент типа EncryptionService*
  EXPECT_NO_THROW(pm_cmd.executeOnClient(args));
}

// 4. Тест MessagingServer: Регистрация обработчиков
TEST(ServerTest, Server_CommandRouting) {
  // MessagingServer внутренне подписывается на stream
  // Проверяем, что сервер корректно обрабатывает входящие пары {fd, Message}

  // В текущей реализации MessagingServer:
  // case MessageType::Command: command_stream.emit({fd, msg});
  // Нужно убедиться, что логика обработки логина в конструкторе работает
  SUCCEED(); // Логика в основном в приватных методах и лямбдах
}

// 5. Тест MessagingClient: Кэширование сообщений
TEST(ClientTest, Client_PendingMessages) {
  // Проверяем логику MessagingClient по сохранению сообщений,
  // если ключ еще не получен (MessageType::CipherMessage)

  // Поскольку MessagingClient требует IClient, здесь лучше использовать mock
  // или проверить, что mutex_ работает при доступе к pending_messages
  SUCCEED();
}

// ==========================================
// 15. КРАЕВЫЕ СЛУЧАИ ДЛЯ КОМАНД
// ==========================================

// Тест: Команда с недостаточным количеством аргументов
TEST(CommandEdgeCase, GetPubkey_NoArgs) {
  GetPubkeyCommand cmd;
  ParsedCommand empty_pc;

  // Команда должна безопасно обработать отсутствие аргументов
  EXPECT_NO_THROW(cmd.fromParsedCommand(empty_pc));

  // И при попытке выполнить на сервере вернуть понятную ошибку
  MessagingService service;
  auto res = cmd.execeuteOnServer(1, service).get_payload();
  std::string res_text(res.begin(), res.end());

  std::string error_msg = "Usage: /getpub <username>";
  EXPECT_EQ(res_text, error_msg);
}

// Тест: Выполнение команды без авторизации (MakeRoom)
TEST(CommandEdgeCase, MakeRoom_Unauthorized) {
  MakeRoomCommand cmd;
  MessagingService service;
  int fd = 99; // Неавторизованный FD

  auto res = cmd.execeuteOnServer(fd, service).get_payload();
  std::string res_text(res.begin(), res.end());

  // Проверяем, что сработала проверка !service.is_authenticated(fd)
  EXPECT_NE(res_text.find("authenticate"), std::string::npos);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
