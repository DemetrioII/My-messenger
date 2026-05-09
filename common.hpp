#pragma once

#include <QApplication>
#include <atomic>
#include <csignal>
#include <iostream>
#include <chrono>
#include <thread>

// UI и Мост
#include "include/services/message_bridge.hpp"
#include "include/ui/chat_window.hpp"
#include "include/ui/login_window.hpp"

#include "include/services/messaging_client.hpp"
#include "include/services/messaging_server.hpp"

// Глобальный флаг для потоков
std::atomic<bool> is_running{true};
inline volatile std::sig_atomic_t g_shutdown_requested = 0;

inline void handle_sigint(int) { g_shutdown_requested = 1; }

// -----------------------------------------------------------
// СЕРВЕР (Оставляем как было, тут консоль ок)
// -----------------------------------------------------------
void start_server() {
  MessagingServer messaging_server;
  messaging_server.start_server(8080);
  std::cout << "Server started on port 8080..." << std::endl;

  std::signal(SIGINT, handle_sigint);

  std::thread server_thread([&messaging_server]() { messaging_server.run(); });
  while (!g_shutdown_requested) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  messaging_server.stop();

  if (server_thread.joinable())
    server_thread.join();
}

// -----------------------------------------------------------
// КЛИЕНТ (Теперь с Qt UI)
// -----------------------------------------------------------
void start_gui_client(int argc, char *argv[]) {
  // 1. Initialize QApplication (main loop)
  QApplication app(argc, argv);

  // 2. Initialize main objects (bridge and windows)
  std::shared_ptr<MessageBridge> bridge = std::make_shared<MessageBridge>();
  LoginWindow *loginWindow = new LoginWindow();
  ChatWindow *chatWindow = nullptr;

  // When user enters we should open the chat window
  QObject::connect(loginWindow, &LoginWindow::loginSuccess,
                   [&](const QString &nickname) {
                     if (chatWindow) {
                       chatWindow->deleteLater();
                     }

                     chatWindow = new ChatWindow(nickname);
                     chatWindow->show();

                     QString loginCmd = "/login " + nickname;
                     bridge->sendToClient(loginCmd);

                     QObject::connect(chatWindow, &ChatWindow::sendMessage,
                                      [&](const std::string &message) {
                                        bridge->sendToClient(
                                            QString::fromStdString(message));
                                      });
                   });

  // Qt крутится в main, а socket read/write будет здесь
  std::thread net_thread([bridge, &chatWindow]() {
    auto client = std::make_shared<MessagingClient>("127.0.0.1", 8080);

    QObject::connect(bridge.get(), &MessageBridge::responseReceived,
                     [&chatWindow](const QString &text) {
                       if (chatWindow) {
                         chatWindow->updateResponse(text);
                       }
                     });

    // Эмуляция/Подключение
    if (!client->init_client("127.0.0.1", 8080)) {
      bridge->postResponse("Ошибка: Сервер недоступен!");
      return;
    }
    bridge->postResponse("Подключено к серверу! 🐧");

    auto ctx = client->get_context();
    ctx->ui_callback = [&bridge](const std::string &text) {
      bridge->postResponse(text);
    };

    QObject::connect(bridge.get(), &MessageBridge::sendToClient,
                     [&client](const QString &text) {
                       client->get_data(text.toStdString());
                     });

    client->run();

    client->get_data("/exit");
  });

  loginWindow->show();
  app.exec();

  // Когда окно закрыли — выходим
  is_running = false;

  if (net_thread.joinable())
    net_thread.join();
}
