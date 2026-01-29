#ifndef CHECKLISTING_H
#define CHECKLISTING_H

#include <QDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QProgressDialog>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>

#include <QRegularExpression>

namespace GOL {

struct RowWidgets {
  QWidget *container;
  QLineEdit *sector;
  QLineEdit *gogo;
  QLineEdit *net;
  QLineEdit *tix;
};

class CheckListing : public QDialog {
  Q_OBJECT

public:
  explicit CheckListing(QWidget *parent = nullptr);

private slots:
  void addRow();
  void process();
  void resetAll();
  void onCalcStock();
  void onImportGogo();
  void onImportTix();
  void onImportNet();

private:
  void setupUI();

  QTextEdit *m_txtOriginal;
  QTextEdit *m_txtResult;
  QVBoxLayout *m_rowsLayout;
  QList<RowWidgets> m_unifiedRows;
  QLabel *m_lblGrand;

  // UI Helpers
  QWidget *createSectionLabel(const QString &text);

  // Background Processing
  void onCalcFinished();
  QFutureWatcher<QString> m_calcWatcher;
  QPushButton *m_btnCalc = nullptr;
};

} // namespace GOL

#endif // CHECKLISTING_H
