#pragma once

#include <QHBoxLayout>
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
  ChatWindow(std::shared_ptr<MessagingClient> client, QWidget *parent = nullptr)
      : QWidget(parent), client_(client) {
    auto *layout = new QVBoxLayout(this);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText("Поиск пользователя...");
    connect(searchEdit_, &QLineEdit::textChanged, this,
            &ChatWindow::onSearchTextChanged);

    resultsList_ = new QListWidget(this);
    connect(resultsList_, &QListWidget::itemClicked, this,
            &ChatWindow::onUserSelected);

    startChatButton_ = new QPushButton("Start Chat", this);
    connect(startChatButton_, &QPushButton::clicked, this,
            &ChatWindow::onStartChatClicked);

    historyView_ = new QTextEdit(this);
    historyView_->setReadOnly(true);

    messageEdit_ = new QLineEdit(this);
    messageEdit_->setPlaceholderText("Введите сообщение...");
    sendButton_ = new QPushButton("Send", this);
    connect(sendButton_, &QPushButton::clicked, this,
            &ChatWindow::onSendClicked);

    auto *topRow = new QHBoxLayout();
    topRow->addWidget(searchEdit_);
    topRow->addWidget(startChatButton_);

    auto *sendRow = new QHBoxLayout();
    sendRow->addWidget(messageEdit_);
    sendRow->addWidget(sendButton_);

    layout->addLayout(topRow);
    layout->addWidget(resultsList_);
    layout->addWidget(historyView_);
    layout->addLayout(sendRow);
  }
  void openChatWith(const std::string &username) {}

signals:
  void sendPlaintextMessage(const std::string &recipient,
                            const std::vector<uint8_t> &payload);

private slots:
  void onSearchTextChanged(const QString &text) { refreshSearchResults(text); }
  void onUserSelected(QListWidgetItem *item) {
    if (!item)
      return;
    currentRecipient_ = item->text().toStdString();
    historyView_->clear();
  }
  void onStartChatClicked() {
    auto txt = searchEdit_->text();
    if (txt.isEmpty())
      return;
    std::string username = txt.toStdString();
    auto info = client_->query_user_info(username);
    if (!info) {
      QMessageBox::warning(this, "User not found",
                           "User is offline or not found");
      return;
    }
  }
  void onSendClicked();

private:
  void refreshSearchResults(const QString &text) {
    resultsList_->clear();
    if (text.isEmpty())
      return;

    auto names = client_->search_online_users(text.toStdString());
    for (auto &n : names) {
      resultsList_->addItem(QString::fromStdString(n));
    }
  }

  std::shared_ptr<MessagingClient> client_;
  QLineEdit *searchEdit_;
  QListWidget *resultsList_;
  QTextEdit *historyView_;
  QLineEdit *messageEdit_;
  QPushButton *sendButton_;
  QPushButton *startChatButton_;

  std::string currentRecipient_;
};
