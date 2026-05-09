#pragma once

#include <QDateTime>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <vector>

#include "include/application/messaging/messaging_client.hpp"

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
  QWidget *makeBubble(const QString &text, bool outgoing);
  void appendBubble(const QString &text, bool outgoing);

  QString userNickname;
  QLabel *titleLabel;
  QScrollArea *historyArea;
  QWidget *historyContent;
  QVBoxLayout *historyLayout;
  QLineEdit *inputField;
  QLabel *statusLabel;
};
