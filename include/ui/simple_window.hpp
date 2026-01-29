#pragma once
#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class MessengerUI : public QWidget {
  Q_OBJECT
public:
  MessengerUI(QWidget *parent = nullptr);

signals:
  void loginAttempt(const std::string &command);

public slots:
  // Теперь это добавляет текст в историю чата
  void updateResponse(const QString &text);

private slots:
  void handleLogin();

  void handleSend();

private:
  QLabel *titleLabel;
  QLineEdit *inputField;
  QTextEdit *historyArea;
  QLabel *statusLabel;
};
