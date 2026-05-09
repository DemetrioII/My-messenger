#include "../../include/ui/login_window.hpp"

#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("LibreMessage");
  setFixedSize(460, 280);

  auto *layout = new QVBoxLayout(this);
  layout->setSpacing(16);
  layout->setContentsMargins(28, 28, 28, 28);

  auto *titleLabel = new QLabel("LibreMessage", this);
  titleLabel->setAlignment(Qt::AlignCenter);
  titleLabel->setObjectName("titleLabel");
  layout->addWidget(titleLabel);

  auto *subtitleLabel =
      new QLabel("Secure chat client for the messenger lab", this);
  subtitleLabel->setAlignment(Qt::AlignCenter);
  subtitleLabel->setObjectName("subtitleLabel");
  layout->addWidget(subtitleLabel);

  nicknameInput = new QLineEdit(this);
  nicknameInput->setPlaceholderText("Nickname");
  nicknameInput->setMaxLength(20);
  nicknameInput->setClearButtonEnabled(true);
  layout->addWidget(nicknameInput);

  loginButton = new QPushButton("Войти", this);
  loginButton->setDefault(true);
  loginButton->setCursor(Qt::PointingHandCursor);
  layout->addWidget(loginButton);

  statusLabel = new QLabel("Ready", this);
  statusLabel->setAlignment(Qt::AlignCenter);
  statusLabel->setObjectName("statusLabel");
  layout->addWidget(statusLabel);

  layout->addStretch();

  connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
  connect(nicknameInput, &QLineEdit::returnPressed, this,
          &LoginWindow::handleLogin);

  setStyleSheet(R"(
    QWidget {
      background: #111318;
      color: #e8eaee;
      font-family: Inter, "Segoe UI", sans-serif;
    }
    QLabel#titleLabel {
      font-size: 30px;
      font-weight: 700;
      letter-spacing: 0.5px;
      color: #f4f7fb;
      margin-top: 4px;
    }
    QLabel#subtitleLabel {
      color: #9aa4b2;
      font-size: 13px;
      margin-bottom: 4px;
    }
    QLabel#statusLabel {
      color: #8b96a6;
      font-size: 13px;
    }
    QLineEdit {
      background: #181c23;
      border: 1px solid #2a3140;
      border-radius: 12px;
      padding: 12px 14px;
      font-size: 14px;
      color: #f4f7fb;
    }
    QLineEdit:focus {
      border: 1px solid #5b8def;
      background: #1b2030;
    }
    QPushButton {
      background: #5b8def;
      border: none;
      border-radius: 12px;
      padding: 12px 16px;
      font-size: 14px;
      font-weight: 600;
      color: white;
    }
    QPushButton:hover {
      background: #6e99f2;
    }
    QPushButton:pressed {
      background: #4f7fd8;
    }
  )");
}

void LoginWindow::handleLogin() {
  QString nickname = nicknameInput->text().trimmed();

  if (nickname.isEmpty()) {
    statusLabel->setText("Nickname cannot be empty");
    statusLabel->setStyleSheet("color: #f38b8b;");
    return;
  }

  emit loginSuccess(nickname);

  this->close();
}
