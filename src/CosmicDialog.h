#ifndef COSMICDIALOG_H
#define COSMICDIALOG_H

#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>


class CosmicDialog : public QDialog {
  Q_OBJECT
public:
  enum Type { Info, Warning, Error, Success };

  explicit CosmicDialog(const QString &title, const QString &message, Type type,
                        QWidget *parent = nullptr);
  static void show(const QString &title, const QString &message, Type type,
                   QWidget *parent = nullptr);

protected:
  void showEvent(QShowEvent *event) override;

private:
  void setupUI(const QString &title, const QString &message, Type type);
  void animateEntry();

  Type m_type;
  QWidget *m_contentFrame;
};

#endif // COSMICDIALOG_H
