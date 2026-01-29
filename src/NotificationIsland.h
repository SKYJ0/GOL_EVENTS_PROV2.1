#ifndef NOTIFICATIONISLAND_H
#define NOTIFICATIONISLAND_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>


namespace GOL {

class NotificationIsland : public QWidget {
  Q_OBJECT

public:
  explicit NotificationIsland(QWidget *parent = nullptr);

  enum Type { Info, Success, Error, Loading };

public slots:
  void showMessage(const QString &message, Type type = Info);
  void hideMessage();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  void updateStyle(Type type);

  QLabel *m_iconLabel;
  QLabel *m_textLabel;
  QHBoxLayout *m_layout;
  QTimer *m_hideTimer;

  // Animation properties
  QPropertyAnimation *m_widthAnim;
  bool m_isLoading;
};

} // namespace GOL

#endif // NOTIFICATIONISLAND_H
