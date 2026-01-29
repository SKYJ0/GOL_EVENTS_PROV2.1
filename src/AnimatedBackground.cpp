#include "AnimatedBackground.h"
#include <QPainter>
#include <QRadialGradient>
#include <QRandomGenerator>

namespace GOL {

AnimatedBackground::AnimatedBackground(QWidget *parent) : QWidget(parent) {
  // setAttribute(Qt::WA_TransparentForMouseEvents); // REMOVED: This blocked
  // all interaction!
  m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, this,
          &AnimatedBackground::updateAnimation);
  m_timer->start(32); // ~30 FPS is enough for background
}

void AnimatedBackground::initOrbs() {
  m_orbs.clear();
  // Create 5-8 random orbs
  int count = 6;
  for (int i = 0; i < count; ++i) {
    Orb orb;
    orb.pos = QPointF(QRandomGenerator::global()->bounded(width()),
                      QRandomGenerator::global()->bounded(height()));
    orb.velocity = QPointF(
        (QRandomGenerator::global()->bounded(20) - 10) / 10.0, // -1.0 to 1.0
        (QRandomGenerator::global()->bounded(20) - 10) / 10.0);
    orb.radius = QRandomGenerator::global()->bounded(200) + 300; // Large

    // Sapphire colors
    int hue = 210 + QRandomGenerator::global()->bounded(40); // Blue-ish
    orb.color = QColor::fromHsl(hue, 150, 50, 40);           // Low alpha

    m_orbs.append(orb);
  }
}

void AnimatedBackground::resizeEvent(QResizeEvent *event) {
  if (m_orbs.isEmpty())
    initOrbs();
  QWidget::resizeEvent(event);
}

void AnimatedBackground::updateAnimation() {
  for (Orb &orb : m_orbs) {
    orb.pos += orb.velocity;

    // Bounce off walls (with buffer)
    if (orb.pos.x() < -100 || orb.pos.x() > width() + 100)
      orb.velocity.setX(-orb.velocity.x());
    if (orb.pos.y() < -100 || orb.pos.y() > height() + 100)
      orb.velocity.setY(-orb.velocity.y());
  }
  update();
}

void AnimatedBackground::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  // Draw base dark background
  p.fillRect(rect(), QColor("#0f172a")); // Slate 900 base

  // Draw Orbs
  for (const Orb &orb : m_orbs) {
    QRadialGradient grad(orb.pos, orb.radius);
    grad.setColorAt(0, orb.color);
    grad.setColorAt(1, Qt::transparent);
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawEllipse(orb.pos, orb.radius, orb.radius);
  }
}

} // namespace GOL
