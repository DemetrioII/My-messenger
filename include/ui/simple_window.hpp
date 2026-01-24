#pragma once
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <QDateTime>

class MessengerUI : public QWidget {
  Q_OBJECT
public:
  MessengerUI(QWidget *parent = nullptr) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    // Заголовок состояния
    titleLabel = new QLabel("<b>Авторизация</b>", this);
    layout->addWidget(titleLabel);

    // Область истории сообщений (скрыта до логина)
    historyArea = new QTextEdit(this);
    historyArea->setReadOnly(true);
    historyArea->setPlaceholderText("Тут будет ваша переписка...");
    layout->addWidget(historyArea);
    historyArea->hide(); // Сначала прячем чат

    // Поле ввода
    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Введите никнейм и нажмите Enter...");
    layout->addWidget(inputField);

    // Статус-бар внизу
    statusLabel = new QLabel("Ожидание подключения...", this);
    layout->addWidget(statusLabel);

    // Соединяем ввод с обработчиком логина
    connect(inputField, &QLineEdit::returnPressed, this, &MessengerUI::handleLogin);
    
    // Настройка внешнего вида (в стиле терминала)
    this->setStyleSheet("background-color: #1e1e1e; color: #dcdcdc; font-family: 'Consolas', 'Monaco', monospace;");
    inputField->setStyleSheet("background-color: #2d2d2d; border: 1px solid #3f3f3f; padding: 5px;");
    historyArea->setStyleSheet("background-color: #252526; border: none;");
  }

signals:
  void loginAttempt(const std::string &command);

public slots:
  // Теперь это добавляет текст в историю чата
  void updateResponse(const QString &text) { 
      QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
      historyArea->append(timestamp + text); 
  }

private slots:
  void handleLogin() {
    QString nickname = inputField->text().trimmed();
    if (nickname.isEmpty()) return;

    std::string loginCommand = "/login " + nickname.toStdString();

    // Переключаем интерфейс в режим чата
    titleLabel->setText("<b>Чат: " + nickname + "</b>");
    statusLabel->setText("Статус: В сети");
    inputField->clear();
    inputField->setPlaceholderText("Напишите сообщение...");
    
    // Показываем историю сообщений
    historyArea->show();

    // Отправляем команду на сервер через мост
    emit loginAttempt(loginCommand);

    // Меняем логику кнопки Enter: теперь она будет отправлять сообщения
    disconnect(inputField, &QLineEdit::returnPressed, this, &MessengerUI::handleLogin);
    connect(inputField, &QLineEdit::returnPressed, this, &MessengerUI::handleSend);
  }

  void handleSend() {
    QString text = inputField->text().trimmed();
    if (text.isEmpty()) return;

    // Эмитим текст в мост (твой MessagingClient::get_data его обработает)
    emit loginAttempt(text.toStdString());
    
    // Добавляем свое сообщение в локальное окно (опционально, если сервер не присылает эхо)
    // updateResponse("Вы: " + text); 
    
    inputField->clear();
  }

private:
  QLabel *titleLabel;
  QLineEdit *inputField;
  QTextEdit *historyArea;
  QLabel *statusLabel;
};
