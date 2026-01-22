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
    inputField->setPlaceholderText("Введите сообщение...");

    statusLabel = new QLabel("Ожидание ввода...", this);
    responseLabel = new QLabel("Тут будет ответ от сервера", this);
    responseLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");

    layout->addWidget(new QLabel("Ваш ввод:"));
    layout->addWidget(inputField);
    layout->addWidget(statusLabel);
    layout->addWidget(new QLabel("Ответ системы:"));
    layout->addWidget(responseLabel);

    connect(inputField, &QLineEdit::returnPressed, this,
            &MessengerUI::handleSend);
  }

signals:
  void sendMessage(const QString &text);

public slots:
  void updateResponse(const QString &text) { responseLabel->setText(text); }

private slots:
  void handleSend() {
    QString text = inputField->text();
    statusLabel->setText("Отправлено: " + text);
    inputField->clear();

    emit sendMessage(text);
  }

private:
  QLineEdit *inputField;
  QLabel *statusLabel;
  QLabel *responseLabel;
};
