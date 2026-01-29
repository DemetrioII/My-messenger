#include "../../include/ui/login_window.hpp"

#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Вход в LibreMessage");
  setFixedSize(400, 200);

  auto *layout = new QVBoxLayout(this);
  layout->setSpacing(15);
  layout->setContentsMargins(30, 30, 30, 30);

  auto *titleLabel = new QLabel("<h2>Добро пожаловать!</h2>", this);
  titleLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(titleLabel);

  nicknameInput = new QLineEdit(this);
  nicknameInput->setPlaceholderText("Введите ваш никнейм...");
  nicknameInput->setMaxLength(20);
  layout->addWidget(nicknameInput);

  loginButton = new QPushButton("Войти", this);
  loginButton->setDefault(true);
  layout->addWidget(loginButton);

  statusLabel = new QLabel("Готов к подключению", this);
  statusLabel->setAlignment(Qt::AlignCenter);
  statusLabel->setStyleSheet("color: #080808;");
  layout->addWidget(statusLabel);

  layout->addStretch();

  connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
  connect(nicknameInput, &QLineEdit::returnPressed, this,
          &LoginWindow::handleLogin);

  setStyleSheet(R"(
				QWidget {
					background-color: #1e1e1e;
					color: #dcdcdc;
					font-family: 'Consolas', 'Monaco', monospace;
				}
				QLineEdit {
					background-color: #2d2d2d;
					border: 1px solid #3f3f3f;
					border-radius: 5px;
					padding: 10px;
					font-size: 14px;
				}
				QLineEdit:focus {
					border: 1px solid #007acc;
				}
				QPushButton {
					background-color: #0e639c;
					border: none;
					border-radius: 5px;
					padding: 10px;
					font-size: 14px;
					font-weight: bold;
				}
				QPushButton:hover {
					background-color: #1177bb;
				}
				QPushButton:pressed {
					background-color: #0d5a8f;
				}
				)");
}

void LoginWindow::handleLogin() {
  QString nickname = nicknameInput->text().trimmed();

  if (nickname.isEmpty()) {
    statusLabel->setText("⚠ Никнейм не может быть пустым!");
    statusLabel->setStyleSheet("color: #f48771;");
    return;
  }

  emit loginSuccess(nickname);

  this->close();
}
