#pragma once
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

class MessengerUI : public QWidget {
  Q_OBJECT
public:
  MessengerUI(QWidget *parent = nullptr) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Введите никнейм и нажмите Enter...");

    statusLabel = new QLabel("Ожидание авторизации...", this);
    responseLabel = new QLabel("Добро пожаловать в мессенджер!", this);
    responseLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");

    layout->addWidget(new QLabel("Логин:"));
    layout->addWidget(inputField);
    layout->addWidget(statusLabel);
    layout->addWidget(responseLabel);

    connect(inputField, &QLineEdit::returnPressed, this,
            &MessengerUI::handleLogin);
  }

signals:
  void sendMessage(const QString &text);

  void loginAttempt(const std::string &command);

public slots:
  void updateResponse(const QString &text) { responseLabel->setText(text); }

private slots:
  void handleSend() {
    QString text = inputField->text();
    if (!text.isEmpty()) {
      emit loginAttempt(text.toStdString());
      inputField->clear();
    }
  }

  void handleLogin() {
    QString nickname = inputField->text().trimmed();
    if (nickname.isEmpty())
      return;

    std::string loginCommand = "/login " + nickname.toStdString();

    statusLabel->setText("Вход под именем: " + nickname);
    inputField->clear();
    inputField->setPlaceholderText("Введите сообщение...");

    emit loginAttempt(loginCommand);

    disconnect(inputField, &QLineEdit::returnPressed, this,
               &MessengerUI::handleLogin);
    connect(inputField, &QLineEdit::returnPressed, this,
            &MessengerUI::handleSend);
  }

private:
  QLineEdit *inputField;
  QLabel *statusLabel;
  QLabel *responseLabel;
};
