#ifndef ANIMATEDBACKGROUND_H
#define ANIMATEDBACKGROUND_H

#include <QColor>
#include <QPointF>
#include <QTimer>
#include <QVector>
#include <QWidget>


namespace GOL {

struct Orb {
  QPointF pos;
  QPointF velocity;
  qreal radius;
  QColor color;
};

class AnimatedBackground : public QWidget {
  Q_OBJECT

public:
  explicit AnimatedBackground(QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void updateAnimation();

private:
  QTimer *m_timer;
  QVector<Orb> m_orbs;
  void initOrbs();
};

} // namespace GOL

#endif // ANIMATEDBACKGROUND_H
