#pragma once

#include <QApplication>
#include <atomic>
#include <iostream>
#include <thread>

// UI и Мост
#include "include/services/message_bridge.hpp"
#include "include/ui/simple_window.hpp"

#include "include/services/messaging_client.hpp"
#include "include/services/messaging_server.hpp"

// Глобальный флаг для потоков
std::atomic<bool> is_running{true};

// -----------------------------------------------------------
// СЕРВЕР (Оставляем как было, тут консоль ок)
// -----------------------------------------------------------
void start_server() {
  MessagingServer messaging_server;
  messaging_server.start_server(8080);
  std::cout << "Server started on port 8080..." << std::endl;
  messaging_server.run();
}

// -----------------------------------------------------------
// КЛИЕНТ (Теперь с Qt UI)
// -----------------------------------------------------------
void start_gui_client(int argc, char *argv[]) {
  // 1. Инициализация Qt (Главный цикл)
  QApplication app(argc, argv);

  // 2. Создаем объекты: Мост и Окно
  MessageBridge bridge;
  MessengerUI window;

  // 3. Соединяем: Когда Мост получает данные -> Окно обновляет текст
  QObject::connect(&bridge, &MessageBridge::responseReceived, &window,
                   &MessengerUI::updateResponse);

  QObject::connect(&window, &MessengerUI::sendMessage, &bridge,
                   &MessageBridge::postSend);

  // 4. Показываем окно
  window.resize(400, 300);
  window.show();

  // 5. ЗАПУСКАЕМ СЕТЬ В ОТДЕЛЬНОМ ПОТОКЕ 🧵
  // Qt крутится в main, а socket read/write будет здесь
  std::thread net_thread([&bridge]() {
    MessagingClient client;

    // Эмуляция/Подключение
    if (!client.init_client("127.0.0.1", 8080)) {
      bridge.postResponse("Ошибка: Сервер недоступен!");
      return;
    }
    bridge.postResponse("Подключено к серверу! 🐧");

    // ВАЖНО:
    // Чтобы клиент мог писать в окно, нам нужно передать ему callback.
    // Добавь в MessagingClient поле: std::function<void(std::string)> on_msg;
    // И вызывай его, когда read() возвращает данные.

    // Пример (псевдокод интеграции):
    /*
    client.on_msg = [&bridge](const std::string& msg) {
        bridge.postResponse(msg);
    };
    */

    QObject::connect(&bridge, &MessageBridge::sendToClient,
                     [&client](const QString &text) {
                       client.get_data(text.toStdString());
                     });

    // Пока просто запустим цикл клиента
    // Если твой client.run() блокирующий, он будет жить здесь
    client.run();
  });

  // Отсоединяем поток, чтобы он жил своей жизнью (Daemon style)
  net_thread.detach();

  // 6. Запуск Event Loop Qt (блокирует этот поток, пока окно открыто)
  app.exec();

  // Когда окно закрыли — выходим
  is_running = false;
}
