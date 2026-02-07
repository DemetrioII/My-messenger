#include "../include/network/transport/peer.hpp"
#include <iostream>
#include <memory>
#include <thread>

int main() {
  // Создаём PeerNodeConnection.
  // Передаём nullptr для socket/loop, пусть фабрика создаёт их внутри.
  auto node = PeerNodeConnection::creat();

  // Колбэк: пришло сообщение
  node->set_message_callback([](int fd, const std::vector<uint8_t> &data) {
    std::string msg(data.begin(), data.end());
    std::cout << "[MSG] from fd " << fd << " : " << msg << "\n";
  });

  // Колбэк: изменение состояния
  node->set_state_callback([](int fd, ConnectionState state) {
    std::cout << "[STATE] fd " << fd << " => "
              << (state == ConnectionState::CONNECTED ? "CONNECTED"
                                                      : "DISCONNECTED")
              << "\n";
  });

  // Колбэк: когда peer подключается
  node->set_peer_connected_callbck([](const std::string &peer_id) {
    std::cout << "[PEER CONNECTED] " << peer_id << "\n";
  });

  // Колбэк: когда peer отключается
  node->set_peer_disconnected_callback([](const std::string &peer_id) {
    std::cout << "[PEER DISCONNECTED] " << peer_id << "\n";
  });

  // Начинаем слушать входящие соединения на порту 5000
  node->start_listening(5000);
  std::cout << "Listening on port 5000...\n";

  // Запускаем цикл обработки событий (event loop)
  node->start_event_loop();

  // Пробуем подключиться к другому узлу
  bool ok = node->connect_to_peer(0, "peer-remote", 5000);
  if (!ok) {
    std::cerr << "Failed to connect to peer-remote\n";
  } else {
    std::cout << "Attempting connection to peer-remote:5000\n";
  }

  // Ждём немного, чтобы дать chance на установку соединения
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // Получаем список активных peer’ов
  auto peers = node->get_connected_peers();
  std::cout << "Active connections: " << peers.size() << "\n";
  for (auto fd : peers) {
    std::cout << " — peer fd " << fd << "\n";
  }

  // Если есть подключённые peer’ы — шлём привет
  if (!peers.empty()) {
    std::string hello = "Hello from " + node->get_node_id();
    node->send_to_peer(peers.front(),
                       std::vector<uint8_t>(hello.begin(), hello.end()));
  }

  // Ждём (напр., для ввода/отладочной активности)
  std::cout << "Press ENTER to quit...\n";
  std::cin.get();

  // Останавливаем event loop
  node->stop_event_loop();

  // Закрываем все соединения
  node->close_all_connections();
  std::cout << "Shutdown complete\n";

  return 0;
}
