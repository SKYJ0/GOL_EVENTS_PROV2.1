#ifndef CALCSTOCK_H
#define CALCSTOCK_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <tuple>
#include <utility>

namespace GOL {

class CalcStock : public QDialog {
  Q_OBJECT

public:
  struct SeatInfo {
    QString raw;
    int num;
    QString suffix;
  };

  struct GroupInfo {
    QString rawStart;
    QString rawEnd;
    int qty;
  };

  explicit CalcStock(QWidget *parent = nullptr);

private slots:
  void browsePath();
  void runCalculation();

public:
  // Logic helpers - made static for reuse
  static std::tuple<QString, QString, QString>
  extractSrsFromFilename(const QString &filename);
  static QString extractFvFromFilename(const QString &filename);
  static std::pair<int, QString> parseSeatDetailed(const QString &seatStr);
  static QList<QList<SeatInfo>> findConsecutiveGroups(QList<SeatInfo> &seats,
                                                      int step);

  // Main static Generator
  static QString generateReportContent(const QString &basePath,
                                       bool oddEvenMode);

private:
  void log(const QString &msg, bool clear = false);

  QLineEdit *m_pathInput;
  QCheckBox *m_oddEvenMode;
  QTextEdit *m_logArea;
};

} // namespace GOL

#endif // CALCSTOCK_H
