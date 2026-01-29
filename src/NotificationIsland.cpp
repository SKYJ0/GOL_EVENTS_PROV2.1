#include "NotificationIsland.h"
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QPainter>


namespace GOL {

NotificationIsland::NotificationIsland(QWidget *parent)
    : QWidget(parent), m_isLoading(false) {

  // Setup Widget
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_TransparentForMouseEvents); // Let clicks pass through if
                                                  // needed, or false to block
  setAttribute(Qt::WA_TransparentForMouseEvents,
               false); // We might want to interact (close it?)

  setFixedHeight(40); // Collapsed height
  setFixedWidth(0);   // Start hidden

  // Layout
  m_layout = new QHBoxLayout(this);
  m_layout->setContentsMargins(15, 0, 15, 0);
  m_layout->setSpacing(10);
  m_layout->setAlignment(Qt::AlignCenter);

  // Icon
  m_iconLabel = new QLabel();
  m_iconLabel->setStyleSheet(
      "font-size: 16px; background: transparent; border: none;");
  m_layout->addWidget(m_iconLabel);

  // Text
  m_textLabel = new QLabel();
  m_textLabel->setStyleSheet("color: white; font-weight: 600; font-size: 13px; "
                             "background: transparent; border: none;");
  m_layout->addWidget(m_textLabel);

  // Shadow
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(20);
  shadow->setColor(QColor(0, 0, 0, 150));
  shadow->setOffset(0, 5);
  setGraphicsEffect(shadow);

  // Timer to auto-hide
  m_hideTimer = new QTimer(this);
  m_hideTimer->setSingleShot(true);
  connect(m_hideTimer, &QTimer::timeout, this,
          &NotificationIsland::hideMessage);

  // Animations
  m_widthAnim = new QPropertyAnimation(this, "minimumWidth");
  m_widthAnim->setDuration(300);
  m_widthAnim->setEasingCurve(QEasingCurve::OutBack);

  hide();
}

void NotificationIsland::showMessage(const QString &message, Type type) {
  m_textLabel->setText(message);
  updateStyle(type);

  // Calculate required width
  QFontMetrics fm(m_textLabel->font());
  int textWidth = fm.horizontalAdvance(message);
  int targetWidth = textWidth + 60; // Padding + Icon

  show();
  m_widthAnim->stop();
  m_widthAnim->setStartValue(width());
  m_widthAnim->setEndValue(targetWidth);
  m_widthAnim->start();

  // Center horizontally roughly (Parent handles positioning usually, but here
  // manually)
  if (parentWidget()) {
    int parentW = parentWidget()->width();
    move((parentW - targetWidth) / 2, 20); // 20px from top
  }

  if (type != Loading) {
    m_hideTimer->start(3000); // Hide after 3s
  } else {
    m_hideTimer->stop();
  }
}

void NotificationIsland::hideMessage() {
  m_widthAnim->stop();
  m_widthAnim->setStartValue(width());
  m_widthAnim->setEndValue(0);
  connect(m_widthAnim, &QPropertyAnimation::finished, this, &QWidget::hide);
  m_widthAnim->start();
}

void NotificationIsland::updateStyle(Type type) {
  switch (type) {
  case Info:
    m_iconLabel->setText("ℹ️");
    break;
  case Success:
    m_iconLabel->setText("✅");
    break;
  case Error:
    m_iconLabel->setText("❌");
    break;
  case Loading:
    m_iconLabel->setText("⏳");
    break;
  }
}

void NotificationIsland::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  // Dynamic Black Capsule
  p.setBrush(QColor("#000000"));
  p.setPen(Qt::NoPen);
  p.drawRoundedRect(rect(), height() / 2, height() / 2);
}

} // namespace GOL
