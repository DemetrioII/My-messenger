#pragma once

#include <QDateTime>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QFrame>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
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
