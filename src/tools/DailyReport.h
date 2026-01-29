#ifndef DAILYREPORT_H
#define DAILYREPORT_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QStringList>
#include <QTextEdit>

namespace GOL {

class DailyReport : public QDialog {
  Q_OBJECT

public:
  explicit DailyReport(QWidget *parent = nullptr);

private slots:
  void addMatch();
  void removeMatch();
  void addCustomTask(); // New slot
  void onActionClicked(const QString &actionName, const QString &section);
  void generateReport();
  void copyToClipboard();
  void saveMatches();
  void loadMatches();
  void saveSettings(); // New: Persist name
  void loadSettings(); // New: Load name

private:
  void setupUI();
  QString getSelectedMatch();
  void updatePreview();

  // UI Components
  QLineEdit *m_nameInput;
  QCheckBox *m_chkPending; // New Status Toggle
  QListWidget *m_matchesList;
  QTextEdit *m_previewArea;
  QComboBox *m_teamSelector; // For header info if needed, or just persist name

  // Data
  // Section Title -> List of entries
  QMap<QString, QStringList> m_reportData;

  // Constants
  const QStringList SERIE_A_TEAMS = {
      "Atalanta",   "Bologna", "Cagliari", "Como",     "Empoli",
      "Fiorentina", "Genoa",   "Inter",    "Juventus", "Lazio",
      "Lecce",      "Milan",   "Monza",    "Napoli",   "Parma",
      "Roma",       "Torino",  "Udinese",  "Venezia",  "Verona"};
};

} // namespace GOL

#endif // DAILYREPORT_H
