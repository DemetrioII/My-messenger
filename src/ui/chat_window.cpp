#include "../../include/ui/chat_window.hpp"

ChatWindow::ChatWindow(const QString &nickname, QWidget *parent)
    : QWidget(parent), userNickname(nickname) {
  setWindowTitle("Libre Message - " + nickname);
  resize(600, 500);

  auto *layout = new QVBoxLayout(this);
  layout->setSpacing(10);
  layout->setContentsMargins(10, 10, 10, 10);

  titleLabel = new QLabel("<b>Чат: " + nickname + "</b>", this);
  titleLabel->setStyleSheet("font-size: 16px; padding: 5px;");
  layout->addWidget(titleLabel);

  historyArea = new QTextEdit(this);
  historyArea->setReadOnly(true);
  historyArea->setPlaceholderText("Здесь будут отображаться сообщения...");
  layout->addWidget(historyArea);

  inputField = new QLineEdit(this);
  inputField->setPlaceholderText("Напишите сообщение...");
  layout->addWidget(inputField);

  statusLabel = new QLabel("Статус: В сети 🟢", this);
  statusLabel->setStyleSheet("color: #4ec9b0; padding: 5px;");
  layout->addWidget(statusLabel);

  connect(inputField, &QLineEdit::returnPressed, this, &ChatWindow::handleSend);

  setStyleSheet(R"(
			QWidget {
				background-color: 1e1e1e;
				color: #000000;
				font-family: 'Consolas', 'Monaco', monospace;
			}
			QLineEdit {
				background-color: #2d2d2d;
				border: 1px solid #3f3f3f;
				border-radius: 5px;
				padding: 8px;
				font-size: 13px;
			}
			QLineEdit:focus {
				border: 1px solid #007acc;
			}
			QTextEdit {
				background-color: 252526;
				border: 1px solid #3f3f3f;
				border-radius: 5px;
				padding: 8px;
				font-size: 13px;
			}
		)");

  updateResponse("🎉 Добро пожаловать в чат, " + nickname + "!");
}

void ChatWindow::updateResponse(const QString &text) {
  QString timestamp = QDateTime::currentDateTime().toString("[hh:mm::ss] ");
  historyArea->append(timestamp + text);
}

void ChatWindow::handleSend() {
  QString text = inputField->text().trimmed();
  if (text.isEmpty())
    return;

  emit sendMessage(text.toStdString());

  inputField->clear();
}
