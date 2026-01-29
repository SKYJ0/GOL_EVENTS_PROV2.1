#include "ToolCard.h"
#include "Utils.h"
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPainter>

namespace GOL {

ToolCard::ToolCard(const QString &title, const QString &desc,
                   const QString &iconPath, const QString &scriptName,
                   bool isLocked, QWidget *parent)
    : QFrame(parent), m_title(title), m_desc(desc), m_iconPath(iconPath),
      m_scriptName(scriptName), m_isLocked(isLocked) {

  setFixedSize(260, 130); // Compact card size
  setCursor(Qt::PointingHandCursor);
  setObjectName("toolCard");
  setStyleSheet("QFrame#toolCard { "
                "  background-color: rgba(255, 255, 255, 0.03); "
                "  border: 1px solid rgba(255, 255, 255, 0.08); "
                "  border-radius: 16px; "
                "} "
                "QFrame#toolCard:hover { "
                "  background-color: rgba(255, 255, 255, 0.06); "
                "  border-color: rgba(255, 255, 255, 0.15); "
                "}");
  // setMouseTracking(true); // Removed as part of revert
  setupUI();      // CRITICAL: Initialize content
  setupOverlay(); // Initialize hover overlay
}

void ToolCard::setupUI() {
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(15, 15, 15, 15);
  mainLayout->setSpacing(12);

  // Left Icon
  m_iconLabel = new QLabel(m_iconPath);
  m_iconLabel->setFixedSize(90, 90);
  m_iconLabel->setAlignment(Qt::AlignCenter);
  m_iconLabel->setObjectName("cardIcon");
  // FORCE font size via stylesheet to override any global styles
  m_iconLabel->setStyleSheet(
      "background: transparent; border: none; font-size: 72px; color: white;");

  // Right Text Container
  QVBoxLayout *textLayout = new QVBoxLayout();
  textLayout->setSpacing(5);
  textLayout->setAlignment(Qt::AlignVCenter);

  m_titleLabel = new QLabel(m_title);
  m_titleLabel->setWordWrap(true);
  m_titleLabel->setObjectName("cardTitle");
  m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: "
                              "white; background: transparent; border: none;");

  m_descLabel = new QLabel(m_desc);
  m_descLabel->setWordWrap(true);
  m_descLabel->setObjectName("cardDesc");
  m_descLabel->setStyleSheet("font-size: 11px; color: #94a3b8; background: "
                             "transparent; border: none;");

  textLayout->addWidget(m_titleLabel);
  textLayout->addWidget(m_descLabel);

  mainLayout->addWidget(m_iconLabel);
  mainLayout->addLayout(textLayout);

  // Pro Badge (Top Right)
  if (m_isLocked) {
    QLabel *proBadge = new QLabel("PRO", this);
    proBadge->setStyleSheet("background-color: #fbbf24; color: #000; "
                            "font-size: 9px; font-weight: bold; "
                            "padding: 2px 4px; border-radius: 4px;");
    proBadge->adjustSize();
    // Absolute positioning to avoid disrupting flow, or add to layout
    // Using absolute for corner placement
    proBadge->move(width() - proBadge->width() - 10, 10);
    // Note: width() might not be valid yet if not shown, but fixed size 260 is
    // set in constructor. Better to align in resizeEvent? Or just rely on fixed
    // size. Let's rely on fixed size for now or move in resizeEvent. Actually,
    // creating it as a member or finding it later is safer. For simplicity,
    // let's keep it here but we might need to reposition if size changes. But
    // ToolCard has setFixedSize(260, 130). So width() is 260.
    proBadge->move(260 - proBadge->width() - 10, 10);
  }

  // Shadow
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(20);
  shadow->setColor(QColor(0, 0, 0, 80));
  shadow->setOffset(0, 4);
  setGraphicsEffect(shadow);
}

void ToolCard::setupOverlay() {
  m_overlay = new QFrame(this);
  m_overlay->setObjectName("cardOverlay");
  m_overlay->setStyleSheet("background-color: rgba(15, 23, 42, 0.7); "
                           "border-radius: 16px; border: none;");
  m_overlay->hide(); // Hidden by default

  // Using absolute positioning approach (resizeEvent handles geometry)

  // Layout for Overlay
  // We want Start Button (Center) and Guide Button (Top Right)

  // Start Button (Big Play or Lock)
  QString btnText = m_isLocked ? "🔒" : "▶";
  m_startBtn = new QPushButton(btnText, m_overlay);
  m_startBtn->setObjectName("cardStartBtn");
  m_startBtn->setFixedSize(60, 60);

  QString borderCol =
      m_isLocked ? "#ef4444" : "rgba(255, 255, 255, 0.5)"; // Red if locked
  QString hoverCol = m_isLocked ? "#ef4444" : "white";

  m_startBtn->setStyleSheet(QString("QPushButton { "
                                    "  background: rgba(255, 255, 255, 0.1); "
                                    "  border: 2px solid %1; "
                                    "  border-radius: 30px; "
                                    "  color: white; "
                                    "  font-size: 24px; "
                                    "  padding-left: 4px; "
                                    "}"
                                    "QPushButton:hover { "
                                    "  background: rgba(255, 255, 255, 0.25); "
                                    "  border-color: %2; "
                                    "  transform: scale(1.1); "
                                    "}")
                                .arg(borderCol, hoverCol));
  connect(m_startBtn, &QPushButton::clicked, [this]() {
    if (!m_isLocked) {
      emit startClicked(m_scriptName);
    }
  });

  // Guide Button (Small Info)
  m_guideBtn = new QPushButton("❔", m_overlay);
  m_guideBtn->setObjectName("cardGuideBtn");
  m_guideBtn->setFixedSize(24, 24);
  m_guideBtn->setStyleSheet("QPushButton { "
                            "  background: rgba(255, 255, 255, 0.1); "
                            "  border: none; "
                            "  border-radius: 12px; "
                            "  color: #cbd5e1; "
                            "  font-size: 12px; "
                            "}"
                            "QPushButton:hover { "
                            "  background: rgba(255, 255, 255, 0.3); "
                            "  color: white; "
                            "}");
  connect(m_guideBtn, &QPushButton::clicked,
          [this]() { emit guideClicked(m_scriptName, m_title); });

  // Since we are not using a layout manager for the overlay internals (to allow
  // absolute positioning relative to overlay), we will set their positions in
  // resizeEvent.
}

void ToolCard::resizeEvent(QResizeEvent *event) {
  QFrame::resizeEvent(event);
  if (m_overlay) {
    m_overlay->setGeometry(rect());

    // Position buttons
    // Start: Center
    m_startBtn->move((width() - m_startBtn->width()) / 2,
                     (height() - m_startBtn->height()) / 2);

    // Guide: Top Right with margin
    m_guideBtn->move(width() - 32, 8);
  }
}

void ToolCard::enterEvent(QEnterEvent *event) {
  QFrame::enterEvent(event);
  m_overlay->raise();
  m_overlay->show();

  // Glow Fade In logic could go here
}

void ToolCard::leaveEvent(QEvent *event) {
  QFrame::leaveEvent(event);
  m_overlay->hide();
}

// Mouse tracking and Paint events removed to restore original look
/*
void ToolCard::mouseMoveEvent(QMouseEvent *event) {
  m_mousePos = event->pos();
  update(); // Repaint for glow movement
  QFrame::mouseMoveEvent(event);
}

void ToolCard::paintEvent(QPaintEvent *event) {
  QFrame::paintEvent(event); // Draw standard styles first

  if (m_mousePos.x() >= 0) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Spotlight Gradient
    QRadialGradient gradient(m_mousePos, 150);         // 150px radius glow
    gradient.setColorAt(0, QColor(255, 255, 255, 40)); // Bright center
    gradient.setColorAt(1, Qt::transparent);

    p.setBrush(gradient);
    p.setPen(Qt::NoPen);
    p.drawRect(rect());

    // Minimal Border Glow
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(59, 130, 246, 100), 2)); // Blue glow border
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);
  }
}
*/

} // namespace GOL
