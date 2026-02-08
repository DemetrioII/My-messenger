#include "../include/network/transport/peer.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

/**
 * Пример 1: PeerNode в режиме сервера
 * Ожидает подключения от других peer'ов
 */
void example_server_mode() {
  std::cout << "=== Example: Server Mode ===" << std::endl;

  // Создание TCP PeerNode
  auto peer = PeerNodeFabric::create_tcp_peer();

  // Установка callback для обработки сообщений
  peer->set_data_callback(
      [](const std::string &ip, const std::vector<uint8_t> &data) {
        std::string message(data.begin(), data.end());
        std::cout << "[From " << ip << "]: " << message << std::endl;
      });

  // Установка callback для новых подключений
  peer->set_peer_connected_callback([](const std::string &ip) {
    std::cout << "[System] Peer connected: " << ip << std::endl;
  });

  // Установка callback для отключений
  peer->set_peer_disconnected_callback([](const std::string &ip) {
    std::cout << "[System] Peer disconnected: " << ip << std::endl;
  });

  // Запуск прослушивания на порту 8080
  peer->start_listening(8080);

  // Запуск event loop в отдельном потоке
  peer->run_event_loop();

  // Основной поток обрабатывает пользовательский ввод
  std::string input;
  std::cout << "Commands: /peers, /quit, or message to broadcast" << std::endl;

  while (std::getline(std::cin, input)) {
    if (input == "/quit") {
      break;
    } else if (input == "/peers") {
      auto peers = peer->get_connected_peers();
      std::cout << "Connected peers (" << peers.size() << "):" << std::endl;
      for (const auto &ip : peers) {
        std::cout << "  - " << ip << std::endl;
      }
    } else {
      // Широковещание сообщения
      std::vector<uint8_t> data(input.begin(), input.end());
      peer->broadcast(data);
    }
  }

  peer->stop();
}

/**
 * Пример 2: PeerNode в режиме клиента
 * Подключается к другому peer'у
 */
void example_client_mode() {
  std::cout << "=== Example: Client Mode ===" << std::endl;

  auto peer = PeerNodeFabric::create_tcp_peer();

  // Обработка входящих сообщений
  peer->set_data_callback(
      [](const std::string &ip, const std::vector<uint8_t> &data) {
        std::string message(data.begin(), data.end());
        std::cout << "[From " << ip << "]: " << message << std::endl;
      });

  // Подключение к серверу
  if (peer->connect_to_peer("127.0.0.1", 8080)) {
    std::cout << "Connected to server" << std::endl;

    // Запуск event loop в отдельном потоке
    peer->run_event_loop();

    // Отправка сообщений
    std::string input;
    std::cout << "Type messages (or /quit to exit):" << std::endl;

    while (std::getline(std::cin, input)) {
      if (input == "/quit") {
        break;
      }

      std::vector<uint8_t> data(input.begin(), input.end());
      peer->send_to_peer("127.0.0.1", data);
    }

    peer->stop();
  } else {
    std::cerr << "Failed to connect to server" << std::endl;
  }
}

/**
 * Пример 3: Гибридный режим - и сервер, и клиент
 */
void example_hybrid_mode(int listen_port, const std::string &connect_ip = "",
                         int connect_port = 0) {
  std::cout << "=== Example: Hybrid Mode ===" << std::endl;

  auto peer = PeerNodeFabric::create_tcp_peer();

  // Callback для входящих сообщений
  peer->set_data_callback(
      [peer](const std::string &ip, const std::vector<uint8_t> &data) {
        std::string message(data.begin(), data.end());
        std::cout << "[" << ip << "]: " << message << std::endl;

        // Эхо-ответ отправителю
        /*std::string reply = "Echo: " + message;
        std::vector<uint8_t> reply_data(reply.begin(), reply.end());
        peer->send_to_peer(ip, reply_data); */
      });

  peer->set_peer_connected_callback(
      [](const std::string &ip) { std::cout << "[+] " << ip << std::endl; });

  peer->set_peer_disconnected_callback(
      [](const std::string &ip) { std::cout << "[-] " << ip << std::endl; });

  // Запуск как сервера
  peer->start_listening(listen_port);

  // Если указан адрес для подключения
  if (!connect_ip.empty() && connect_port > 0) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    peer->connect_to_peer(connect_ip, connect_port);
  }

  // Запуск event loop
  std::thread event_thread([peer]() { peer->run_event_loop(); });

  std::cout << "Commands:" << std::endl;
  std::cout << "  /peers - list connected peers" << std::endl;
  std::cout << "  /connect <ip> <port> - connect to peer" << std::endl;
  std::cout << "  /disconnect <ip> - disconnect from peer" << std::endl;
  std::cout << "  /quit - exit" << std::endl;
  std::cout << "  <message> - broadcast to all" << std::endl;

  std::string input;
  while (std::getline(std::cin, input)) {
    if (input == "/quit") {
      break;
    } else if (input == "/peers") {
      auto peers = peer->get_connected_peers();
      std::cout << "Connected peers (" << peers.size() << "):" << std::endl;
      for (const auto &ip : peers) {
        std::cout << "  - " << ip << std::endl;
      }
    } else if (input.substr(0, 8) == "/connect") {
      // Парсинг: /connect <ip> <port>
      size_t first_space = input.find(' ', 9);
      if (first_space != std::string::npos) {
        std::string ip = input.substr(9, first_space - 9);
        int port = std::stoi(input.substr(first_space + 1));
        peer->connect_to_peer(ip, port);
      } else {
        std::cout << "Usage: /connect <ip> <port>" << std::endl;
      }
    } else if (input.substr(0, 11) == "/disconnect") {
      std::string ip = input.substr(12);
      peer->disconnect_from_peer_by_ip(ip);
    } else {
      // Широковещание
      std::vector<uint8_t> data(input.begin(), input.end());
      peer->broadcast(data);
    }
  }

  peer->stop();
  event_thread.join();
}

/**
 * Пример 4: P2P чат
 */
class P2PChatNode {
private:
  std::shared_ptr<PeerNode> peer;
  std::string node_name;
  std::thread event_thread;

public:
  P2PChatNode(const std::string &name, int listen_port)
      : node_name(name), peer(PeerNodeFabric::create_tcp_peer()) {

    // Обработка сообщений
    peer->set_data_callback(
        [this](const std::string &ip, const std::vector<uint8_t> &data) {
          std::string message(data.begin(), data.end());
          std::cout << message << std::endl;
        });

    peer->set_peer_connected_callback([this](const std::string &ip) {
      std::cout << "[System] " << ip << " joined the chat" << std::endl;

      // Отправка приветствия
      std::string greeting = "[" + node_name + "] Hello from " + node_name;
      std::vector<uint8_t> data(greeting.begin(), greeting.end());
      peer->send_to_peer(ip, data);
    });

    peer->set_peer_disconnected_callback([](const std::string &ip) {
      std::cout << "[System] " << ip << " left the chat" << std::endl;
    });

    // Запуск прослушивания
    peer->start_listening(listen_port);
    std::cout << "Chat node '" << node_name << "' started on port "
              << listen_port << std::endl;
  }

  void start() {
    event_thread = std::thread([this]() { peer->run_event_loop(); });
  }

  void connect_to(const std::string &ip, int port) {
    peer->connect_to_peer(ip, port);
  }

  void send_message(const std::string &message) {
    std::string formatted = "[" + node_name + "] " + message;
    std::vector<uint8_t> data(formatted.begin(), formatted.end());
    peer->broadcast(data);
  }

  void stop() {
    peer->stop();
    if (event_thread.joinable()) {
      event_thread.join();
    }
  }

  ~P2PChatNode() { stop(); }
};

/**
 * Main функция с примерами
 */
int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <mode> [options]" << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  server               - Run as server only" << std::endl;
    std::cout << "  client               - Run as client only" << std::endl;
    std::cout << "  hybrid <port>        - Run as hybrid (server+client)"
              << std::endl;
    std::cout << "  chat <name> <port>   - Run P2P chat node" << std::endl;
    return 1;
  }

  std::string mode = argv[1];

  try {
    if (mode == "server") {
      example_server_mode();
    } else if (mode == "client") {
      example_client_mode();
    } else if (mode == "hybrid") {
      if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " hybrid <listen_port> [connect_ip "
                     "connect_port]"
                  << std::endl;
        return 1;
      }
      int listen_port = std::stoi(argv[2]);
      std::string connect_ip = argc > 3 ? argv[3] : "";
      int connect_port = argc > 4 ? std::stoi(argv[4]) : 0;
      example_hybrid_mode(listen_port, connect_ip, connect_port);
    } else if (mode == "chat") {
      if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " chat <name> <port>" << std::endl;
        return 1;
      }
      std::string name = argv[2];
      int port = std::stoi(argv[3]);

      P2PChatNode chat(name, port);
      chat.start();

      // Опционально подключение к другому узлу
      if (argc > 5) {
        std::string peer_ip = argv[4];
        int peer_port = std::stoi(argv[5]);
        chat.connect_to(peer_ip, peer_port);
      }

      std::cout << "Type messages (or /quit to exit):" << std::endl;
      std::string input;
      while (std::getline(std::cin, input)) {
        if (input == "/quit")
          break;
        chat.send_message(input);
      }

      chat.stop();
    } else {
      std::cerr << "Unknown mode: " << mode << std::endl;
      return 1;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
