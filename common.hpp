#include "include/services/messageing_service.hpp"
#include "include/services/messaging_client.hpp"
#include "include/services/messaging_server.hpp"

void start_server() {
  MessagingServer messaging_server;
  messaging_server.start_server(8080);
  messaging_server.run();
}

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

// Флаг для корректного завершения потоков
std::atomic<bool> is_running{true};

void input_thread_func(MessagingClient &client) {
  std::string input;
  std::cout << "--- Режим чата запущен. Введите сообщение и нажмите Enter ---"
            << std::endl;

  while (is_running) {
    std::cout << "> " << std::flush;
    if (!std::getline(std::cin, input)) {
      is_running = false;
      break;
    }

    if (input == "/exit") {
      client.get_data(input);
      is_running = false;
      break;
    }

    // Вызываем твою функцию-обработчик
    // Она упакует строку в Message, пропустит через Parser и отправит в сокет
    client.get_data(input);
  }
}

void conn_client() {
  MessagingClient messaging_client;

  // 1. Инициализируем соединение
  if (!messaging_client.init_client("127.0.0.1", 8080)) {
    std::cerr << "Не удалось подключиться к серверу!" << std::endl;
    return;
  }

  // 2. Запускаем поток ввода, передавая ссылку на клиент
  std::thread input_worker(input_thread_func, std::ref(messaging_client));

  // 3. Основной поток отдаем под сетевой Event Loop
  // Он будет крутиться здесь и вызывать callback-и при получении данных

  // Вызываем run_once (через твой сервис или напрямую у TCPClient)
  // Чтобы сетевой поток не "ел" 100% CPU, ставим небольшой таймаут
  messaging_client.run();

  // Ждем завершения потока ввода перед выходом
  if (input_worker.joinable()) {
    input_worker.join();
  }
  std::cout << "Клиент остановлен." << std::endl;
}
