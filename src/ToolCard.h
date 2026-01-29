#ifndef TOOLCARD_H
#define TOOLCARD_H

#include <QEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>

namespace GOL {

class ToolCard : public QFrame {
  Q_OBJECT

public:
  explicit ToolCard(const QString &title, const QString &desc,
                    const QString &iconPath, const QString &scriptName,
                    bool isLocked = false, QWidget *parent = nullptr);

signals:
  void startClicked(const QString &scriptName);
  void guideClicked(const QString &scriptName, const QString &title);

protected:
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  // void mouseMoveEvent(QMouseEvent *event) override;
  // void paintEvent(QPaintEvent *event) override;

private:
  void setupUI();
  void setupOverlay();

  QPoint m_mousePos = QPoint(-1, -1);
  QString m_title;
  QString m_desc;
  QString m_iconPath; // Can be emoji or file path
  QString m_scriptName;
  bool m_isLocked;

  // UI Elements
  QLabel *m_iconLabel;
  QLabel *m_titleLabel;
  QLabel *m_descLabel;

  // Hover Overlay
  QFrame *m_overlay;
  QPushButton *m_startBtn;
  QPushButton *m_guideBtn;

  // Animations
  QPropertyAnimation *m_fadeAnim;
};

} // namespace GOL

#endif // TOOLCARD_H
