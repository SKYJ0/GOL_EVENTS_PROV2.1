#include "CosmicDialog.h"
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QScreen>

CosmicDialog::CosmicDialog(const QString &title, const QString &message,
                           Type type, QWidget *parent)
    : QDialog(parent), m_type(type) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog |
                 Qt::WindowStaysOnTopHint | Qt::Tool);
  setAttribute(Qt::WA_TranslucentBackground);
  setModal(true); // Enforce input blocking
  setupUI(title, message, type);
}

void CosmicDialog::show(const QString &title, const QString &message, Type type,
                        QWidget *parent) {
  CosmicDialog dlg(title, message, type, parent);
  dlg.exec();
}

void CosmicDialog::setupUI(const QString &title, const QString &message,
                           Type type) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(10, 10, 10, 10); // Shadow margin

  // Glass Background Frame
  m_contentFrame = new QWidget(this);
  m_contentFrame->setObjectName("cosmicDialogFrame");
  m_contentFrame->setStyleSheet(
      "QWidget#cosmicDialogFrame {"
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(30, "
      "27, 75, 0.95), stop:1 rgba(15, 23, 42, 0.98));"
      "  border: 1px solid rgba(255, 255, 255, 0.1);"
      "  border-radius: 16px;"
      "}");

  // Glow Effect based on Type
  QGraphicsDropShadowEffect *glow = new QGraphicsDropShadowEffect(this);
  glow->setBlurRadius(30);
  glow->setOffset(0, 0);
  QColor glowColor;
  QString typeIcon;
  QString btnStyle;

  switch (type) {
  case Error:
    glowColor = QColor(239, 68, 68, 100); // Red
    typeIcon = "🛑";                      // Replace with image if available
    btnStyle = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 "
               "#ef4444, stop:1 #b91c1c);";
    break;
  case Warning:
    glowColor = QColor(245, 158, 11, 100); // Orange
    typeIcon = "⚠️";
    btnStyle = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 "
               "#f59e0b, stop:1 #d97706);";
    break;
  case Success:
    glowColor = QColor(16, 185, 129, 100); // Green
    typeIcon = "✅";
    btnStyle = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 "
               "#10b981, stop:1 #059669);";
    break;
  case Info:
  default:
    glowColor = QColor(59, 130, 246, 100); // Blue
    typeIcon = "ℹ️";
    btnStyle = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 "
               "#3b82f6, stop:1 #2563eb);";
    break;
  }
  glow->setColor(glowColor);
  m_contentFrame->setGraphicsEffect(glow);

  QVBoxLayout *contentLayout = new QVBoxLayout(m_contentFrame);
  contentLayout->setContentsMargins(30, 30, 30, 30);
  contentLayout->setSpacing(20);

  // Header (Icon + Title)
  QHBoxLayout *headerLayout = new QHBoxLayout();
  QLabel *iconLabel = new QLabel(typeIcon);
  iconLabel->setStyleSheet(
      "font-size: 32px; background: transparent; border: none;");

  QLabel *titleLabel = new QLabel(title);
  titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: white; "
                            "background: transparent; border: none;");

  // Close X
  QPushButton *closeX = new QPushButton("✕");
  closeX->setFixedSize(24, 24);
  closeX->setCursor(Qt::PointingHandCursor);
  closeX->setStyleSheet(
      "QPushButton { color: rgba(255,255,255,0.5); background: transparent; "
      "border: none; font-weight: bold; } QPushButton:hover { color: white; }");
  connect(closeX, &QPushButton::clicked, this, &CosmicDialog::reject);

  headerLayout->addWidget(iconLabel);
  headerLayout->addWidget(titleLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(closeX, 0, Qt::AlignTop);

  contentLayout->addLayout(headerLayout);

  // Message
  QLabel *msgLabel = new QLabel(message);
  msgLabel->setWordWrap(true);
  msgLabel->setStyleSheet("font-size: 14px; color: #cbd5e1; line-height: 1.5; "
                          "background: transparent; border: none;");
  contentLayout->addWidget(msgLabel);

  // Action Button
  QPushButton *actionBtn = new QPushButton("OK");
  if (type == Error)
    actionBtn->setText("Retry"); // Match reference
  actionBtn->setCursor(Qt::PointingHandCursor);
  actionBtn->setFixedHeight(40);
  actionBtn->setStyleSheet(
      QString("QPushButton { %1 color: white; border: none; border-radius: "
              "8px; font-weight: bold; font-size: 14px; } "
              "QPushButton:hover { opacity: 0.9; }")
          .arg(btnStyle));
  connect(actionBtn, &QPushButton::clicked, this, &CosmicDialog::accept);

  contentLayout->addWidget(actionBtn);

  mainLayout->addWidget(m_contentFrame);

  // Size
  setFixedSize(400, 250);
}

void CosmicDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  this->raise();
  this->activateWindow();
  animateEntry();
}

void CosmicDialog::animateEntry() {
  // Zoom In + Fade In
  QPropertyAnimation *scaleAnim = new QPropertyAnimation(this, "geometry");
  QRect endRect = geometry();
  QRect startRect = endRect;
  startRect.setWidth(endRect.width() * 0.8);
  startRect.setHeight(endRect.height() * 0.8);
  startRect.moveCenter(endRect.center());

  scaleAnim->setDuration(300);
  scaleAnim->setStartValue(startRect);
  scaleAnim->setEndValue(endRect);
  scaleAnim->setEasingCurve(QEasingCurve::OutBack);

  QGraphicsOpacityEffect *opacity = new QGraphicsOpacityEffect(this);
  setGraphicsEffect(opacity);
  QPropertyAnimation *fadeAnim = new QPropertyAnimation(opacity, "opacity");
  fadeAnim->setDuration(250);
  fadeAnim->setStartValue(0.0);
  fadeAnim->setEndValue(1.0);

  scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);
  fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}
