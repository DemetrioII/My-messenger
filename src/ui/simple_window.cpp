#include "../../../include/ui/simple_window.hpp"

MessengerUI::MessengerUI(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setSpacing(14);
  layout->setContentsMargins(20, 20, 20, 20);

  // Заголовок состояния
  titleLabel = new QLabel("<b>Авторизация</b>", this);
  titleLabel->setStyleSheet("font-size: 20px; font-weight: 700;");
  layout->addWidget(titleLabel);

  // Область истории сообщений (скрыта до логина)
  historyArea = new QTextEdit(this);
  historyArea->setReadOnly(true);
  historyArea->setPlaceholderText("Тут будет ваша переписка...");
  historyArea->setStyleSheet(
      "background: #121821; border: 1px solid #2a3342; color: #f4f7fb; "
      "border-radius: 16px; padding: 14px; font-size: 14px;");
  layout->addWidget(historyArea);
  historyArea->hide(); // Сначала прячем чат

  // Поле ввода
  inputField = new QLineEdit(this);
  inputField->setPlaceholderText("Введите никнейм и нажмите Enter...");
  inputField->setClearButtonEnabled(true);
  layout->addWidget(inputField);

  // Статус-бар внизу
  statusLabel = new QLabel("Ожидание подключения...", this);
  statusLabel->setStyleSheet("color: #8ca0b8;");
  layout->addWidget(statusLabel);

  // Соединяем ввод с обработчиком логина
  connect(inputField, &QLineEdit::returnPressed, this,
          &MessengerUI::handleLogin);

  // Настройка внешнего вида (в стиле терминала)
  this->setStyleSheet(
      "background: #0f131a; color: #eef2f7; font-family: Inter, 'Segoe UI', sans-serif;");
  inputField->setStyleSheet(
      "background: #171d27; border: 1px solid #2b3444; border-radius: 14px; "
      "padding: 12px 14px; color: #f4f7fb;");
}

void MessengerUI::updateResponse(const QString &text) {
  QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
  historyArea->append(timestamp + text);
}

void MessengerUI::handleLogin() {
  QString nickname = inputField->text().trimmed();
  if (nickname.isEmpty())
    return;

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
  disconnect(inputField, &QLineEdit::returnPressed, this,
             &MessengerUI::handleLogin);
  connect(inputField, &QLineEdit::returnPressed, this,
          &MessengerUI::handleSend);
}

void MessengerUI::handleSend() {
  QString text = inputField->text().trimmed();
  if (text.isEmpty())
    return;

  // Эмитим текст в мост (твой MessagingClient::get_data его обработает)
  emit loginAttempt(text.toStdString());

  // Добавляем свое сообщение в локальное окно (опционально, если сервер не
  // присылает эхо) updateResponse("Вы: " + text);

  inputField->clear();
}
