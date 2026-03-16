#pragma once

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <vector>

#include "../services/messaging_client.hpp"

class ChatWindow : public QWidget {
  Q_OBJECT

public:
  explicit ChatWindow(const QString &nickname, QWidget *parent = nullptr);
  void updateResponse(const QString &text);

signals:
  void sendMessage(const std::string &message);

private slots:
  void handleSend();

private:
  QString userNickname;
  QLabel *titleLabel;
  QTextEdit *historyArea;
  QLineEdit *inputField;
  QLabel *statusLabel;
};
