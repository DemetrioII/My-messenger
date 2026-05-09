#include "include/ui/chat_window.hpp"

#include <QAbstractAnimation>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>

namespace {
QString htmlEscape(const QString &text) { return text.toHtmlEscaped(); }
} // namespace

ChatWindow::ChatWindow(const QString &nickname, QWidget *parent)
    : QWidget(parent), userNickname(nickname) {
  setWindowTitle("LibreMessage • " + nickname);
  resize(980, 720);

  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(20, 20, 20, 20);
  root->setSpacing(16);

  auto *topBar = new QHBoxLayout();
  topBar->setSpacing(12);

  auto *avatar = new QLabel(nickname.left(1).toUpper(), this);
  avatar->setObjectName("avatarLabel");
  avatar->setAlignment(Qt::AlignCenter);
  avatar->setFixedSize(44, 44);
  topBar->addWidget(avatar);

  auto *titles = new QVBoxLayout();
  titles->setSpacing(2);
  titleLabel = new QLabel("Connected as " + nickname, this);
  titleLabel->setObjectName("titleLabel");
  auto *subtitle = new QLabel("Secure session ready", this);
  subtitle->setObjectName("subtitleLabel");
  titles->addWidget(titleLabel);
  titles->addWidget(subtitle);
  topBar->addLayout(titles);
  topBar->addStretch();

  statusLabel = new QLabel("Online", this);
  statusLabel->setObjectName("statusPill");
  statusLabel->setAlignment(Qt::AlignCenter);
  statusLabel->setFixedHeight(32);
  topBar->addWidget(statusLabel);

  root->addLayout(topBar);

  historyArea = new QScrollArea(this);
  historyArea->setWidgetResizable(true);
  historyArea->setFrameShape(QFrame::NoFrame);
  historyArea->setObjectName("historyArea");

  historyContent = new QWidget(historyArea);
  historyLayout = new QVBoxLayout(historyContent);
  historyLayout->setContentsMargins(12, 12, 12, 12);
  historyLayout->setSpacing(12);
  historyLayout->addStretch();

  historyArea->setWidget(historyContent);
  root->addWidget(historyArea, 1);

  auto *composer = new QHBoxLayout();
  composer->setSpacing(10);

  inputField = new QLineEdit(this);
  inputField->setPlaceholderText("Type a message and press Enter");
  inputField->setClearButtonEnabled(true);
  inputField->setObjectName("composerInput");
  composer->addWidget(inputField, 1);

  auto *sendButton = new QPushButton("Send", this);
  sendButton->setObjectName("sendButton");
  sendButton->setCursor(Qt::PointingHandCursor);
  composer->addWidget(sendButton);

  root->addLayout(composer);

  connect(inputField, &QLineEdit::returnPressed, this, &ChatWindow::handleSend);
  connect(sendButton, &QPushButton::clicked, this, &ChatWindow::handleSend);

  setStyleSheet(R"(
    QWidget {
      background: #0c1016;
      color: #eef2f7;
      font-family: Inter, "Segoe UI", sans-serif;
    }
    QLabel#titleLabel {
      font-size: 22px;
      font-weight: 700;
      color: #f5f7fb;
    }
    QLabel#subtitleLabel {
      font-size: 12px;
      color: #8b96a6;
    }
    QLabel#statusPill {
      background: #182131;
      border: 1px solid #2a3648;
      border-radius: 16px;
      color: #9fb4d6;
      padding: 0 14px;
      font-size: 12px;
    }
    QLabel#avatarLabel {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                  stop:0 #5b8def, stop:1 #8b5cf6);
      color: white;
      border-radius: 22px;
      font-size: 18px;
      font-weight: 700;
    }
    QScrollArea#historyArea {
      background: #0f141b;
      border: 1px solid #202a38;
      border-radius: 18px;
    }
    QWidget#historyContent {
      background: transparent; 
    }
    QWidget#messageRow {
      background: transparent;
    }
    QLabel#inBubble, QLabel#outBubble {
      padding: 12px 14px;
      border-radius: 16px;
      font-size: 14px;
      line-height: 1.35;
      color: #eef2f7;
    }
    QLabel#inBubble {
      background: #151b24;
      border: 1px solid #263043;
    }
    QLabel#outBubble {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                  stop:0 #5b8def, stop:1 #7c5cff);
      border: 1px solid #6d85f7;
    }
    QLineEdit#composerInput {
      background: #151b24;
      border: 1px solid #253042;
      border-radius: 16px;
      padding: 14px 16px;
      font-size: 14px;
      color: #f4f7fb;
    }
    QLineEdit#composerInput:focus {
      border: 1px solid #5b8def;
      background: #182131;
    }
    QPushButton#sendButton {
      background: #5b8def;
      border: none;
      border-radius: 16px;
      padding: 14px 22px;
      font-size: 14px;
      font-weight: 700;
      color: white;
      min-width: 96px;
    }
    QPushButton#sendButton:hover {
      background: #6e99f2;
    }
    QPushButton#sendButton:pressed {
      background: #4f7fd8;
    }
  )");

  appendBubble("Welcome, " + nickname + "!", false);
}

QWidget *ChatWindow::makeBubble(const QString &text, bool outgoing) {
  auto *row = new QWidget(historyContent);
  auto *rowLayout = new QHBoxLayout(row);
  rowLayout->setContentsMargins(0, 0, 0, 0);
  rowLayout->setSpacing(0);

  auto *bubble = new QLabel(text, row);
  bubble->setWordWrap(true);
  bubble->setTextFormat(Qt::RichText);
  bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
  bubble->setObjectName(outgoing ? "outBubble" : "inBubble");
  bubble->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  bubble->setMaximumWidth(620);
  bubble->setContentsMargins(0, 0, 0, 0);

  auto *bubbleWrapper = new QWidget(row);
  auto *bubbleLayout = new QHBoxLayout(bubbleWrapper);
  bubbleLayout->setContentsMargins(0, 0, 0, 0);
  bubbleLayout->setSpacing(0);
  bubbleLayout->addWidget(bubble);

  if (outgoing) {
    rowLayout->addStretch();
    rowLayout->addWidget(bubbleWrapper, 0, Qt::AlignRight);
  } else {
    rowLayout->addWidget(bubbleWrapper, 0, Qt::AlignLeft);
    rowLayout->addStretch();
  }

  row->setObjectName("messageRow");
  return row;
}

void ChatWindow::appendBubble(const QString &text, bool outgoing) {
  auto *row = makeBubble(text, outgoing);
  row->setGraphicsEffect(new QGraphicsOpacityEffect(row));
  auto *effect = qobject_cast<QGraphicsOpacityEffect *>(row->graphicsEffect());
  if (effect)
    effect->setOpacity(0.0);

  historyLayout->insertWidget(historyLayout->count() - 1, row);

  auto *anim = new QPropertyAnimation(effect, "opacity", row);
  anim->setDuration(180);
  anim->setStartValue(0.0);
  anim->setEndValue(1.0);
  anim->start(QAbstractAnimation::DeleteWhenStopped);

  QTimer::singleShot(0, this, [this]() {
    historyArea->verticalScrollBar()->setValue(
        historyArea->verticalScrollBar()->maximum());
  });
}

void ChatWindow::updateResponse(const QString &text) {
  QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss]");
  appendBubble(timestamp + " " + htmlEscape(text), false);
}

void ChatWindow::handleSend() {
  QString text = inputField->text().trimmed();
  if (text.isEmpty())
    return;

  appendBubble(QDateTime::currentDateTime().toString("[hh:mm:ss] ") +
                   htmlEscape(text),
               true);
  emit sendMessage(text.toStdString());
  inputField->clear();
}
