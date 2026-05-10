#pragma once
#include <QObject>
#include <QString>
#include <iostream>
#include <string>

class MessageBridge : public QObject {
  Q_OBJECT
public:
  explicit MessageBridge(QObject *parent = nullptr) : QObject(parent) {}

  std::function<void(const std::string &)> onOutboundMessage;

  void postResponse(const std::string &data) {
    emit responseReceived(QString::fromStdString(data));
  }

  void postSend(const QString &text) { emit sendToClient(text); }

  ~MessageBridge() override {}

public slots:
  void sendToServer(const std::string &data) {
    if (onOutboundMessage) {
      onOutboundMessage(data);
    }
  }

signals:
  // Только объявление! Реализацию напишет MOC.
  void responseReceived(const QString &text);
  void sendToClient(const QString &text);
};
