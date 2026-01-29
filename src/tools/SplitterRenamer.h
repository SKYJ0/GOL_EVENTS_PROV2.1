#ifndef SPLITTERRENAMER_H
#define SPLITTERRENAMER_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

namespace GOL {

class SplitterRenamer : public QDialog {
  Q_OBJECT

public:
  explicit SplitterRenamer(QWidget *parent = nullptr);

private slots:
  void browseFolder();
  void startProcess();

private:
  void log(const QString &msg);
  struct TicketInfo {
    QString s, r, st;
    bool valid = false;
  };
  TicketInfo extractTicketInfo(const QString &pdfPath, int pageNum,
                               const TicketInfo *hint = nullptr);
  TicketInfo extractInfoFromFilename(const QString &filename);
  bool isAcMilanTicket(const QString &filename);

  QLineEdit *m_pathEdit;
  QPushButton *m_btnBrowse;
  QPushButton *m_btnRun;
  QTextEdit *m_logArea;
};

} // namespace GOL

#endif // SPLITTERRENAMER_H
