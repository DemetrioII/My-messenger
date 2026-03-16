#pragma once
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class LoginWindow : public QWidget {
  Q_OBJECT

public:
  explicit LoginWindow(QWidget *parent = nullptr);

signals:
  void loginSuccess(const QString &nickname);

private slots:
  void handleLogin();

private:
  QLineEdit *nicknameInput;
  QLabel *statusLabel;
  QPushButton *loginButton;
};
