#include "include/app/runtime.hpp"
#include "include/app/config.hpp"

#include <QApplication>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "include/application/messaging/message_bridge.hpp"
#include "include/application/messaging/messaging_client.hpp"
#include "include/application/messaging/messaging_server.hpp"
#include "include/infrastructure/logging/logger.hpp"
#include "include/ui/chat_window.hpp"
#include "include/ui/login_window.hpp"

std::atomic<bool> is_running{true};
inline volatile std::sig_atomic_t g_shutdown_requested = 0;

inline void handle_sigint(int) { g_shutdown_requested = 1; }

void start_server() {
  messenger::log::init();
  AppConfig config;
  MessagingServer messaging_server(config);
  messaging_server.start_server();
  messenger::log::info("Server started on " + config.server_host + ":" +
                       std::to_string(config.server_port));

  std::signal(SIGINT, handle_sigint);

  std::thread server_thread([&messaging_server]() { messaging_server.run(); });
  while (!g_shutdown_requested) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  messaging_server.stop();

  if (server_thread.joinable())
    server_thread.join();
}

void start_gui_client(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(true);
  messenger::log::init();

  auto bridge = std::make_shared<MessageBridge>();
  QObject uiBridge; // гарантирует queued delivery в GUI-поток
  LoginWindow *loginWindow = new LoginWindow();
  ChatWindow *chatWindow = nullptr;
  AppConfig config;
  auto client = std::make_shared<MessagingClient>(config);

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

  QObject::connect(
      bridge.get(), &MessageBridge::responseReceived, &uiBridge,
      [&](const QString &text) {
        if (chatWindow) {
          chatWindow->updateResponse(text);
        }
      },
      Qt::QueuedConnection);

  QObject::connect(
      bridge.get(), &MessageBridge::sendToClient, &uiBridge,
      [&client](const QString &text) { client->get_data(text.toStdString()); },
      Qt::QueuedConnection);

  std::thread net_thread([bridge, &chatWindow, client]() {
    if (!client->init_client()) {
      messenger::log::error("Client failed to connect to server");
      bridge->postResponse("Ошибка: Сервер недоступен!");
      return;
    }
    messenger::log::info("Client connected to server");
    bridge->postResponse("Подключено к серверу! 🐧");

    auto ctx = client->get_context();
    ctx->ui_callback = [&bridge](const std::string &text) {
      bridge->postResponse(text);
    };

    client->run();
  });

  QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
    if (client) {
      client->stop();
    }
  });

  loginWindow->show();
  app.exec();

  is_running = false;

  if (net_thread.joinable())
    net_thread.join();
}
