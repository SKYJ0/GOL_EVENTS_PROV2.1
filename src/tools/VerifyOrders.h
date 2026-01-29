#ifndef VERIFYORDERS_H
#define VERIFYORDERS_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QTextEdit>


namespace GOL {

class VerifyOrders : public QDialog {
  Q_OBJECT

public:
  explicit VerifyOrders(QWidget *parent = nullptr);

private slots:
  void browseFolder();
  void selectSalesFiles();
  void startScan();

private:
  void log(const QString &msg);
  QString cleanHeader(const QString &h);
  QStringList extractFromCsv(const QString &path);
  QStringList extractFromExcel(const QString &path);
  // Note: Excel extraction requires extra libs, for now we support CSV
  // prominently

  QLineEdit *m_pathEdit;
  QStringList m_salesFiles;
  QPushButton *m_btnBrowse;
  QPushButton *m_btnSales;
  QPushButton *m_btnRun;
  QTextEdit *m_logArea;

  const QStringList ALLOWED_COLUMNS = {"id", "transactionid", "order id",
                                       "orderid"};
};

} // namespace GOL

#endif // VERIFYORDERS_H
