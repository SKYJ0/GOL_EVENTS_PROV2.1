#ifndef STOCKREPORT_H
#define STOCKREPORT_H

#include <QDialog>
#include <QDir>
#include <QLineEdit>
#include <QMap>
#include <QPair>
#include <QPushButton>
#include <QSet>
#include <QTextEdit>
#include <tuple>


namespace GOL {

class StockReport : public QDialog {
  Q_OBJECT

public:
  explicit StockReport(QWidget *parent = nullptr);

private slots:
  void browseFolder();
  void startAnalysis();

private:
  void log(const QString &msg);

  // Core Logic
  struct StockInfo {
    int count = 0;
    QSet<QString> files;
  };

  struct OrderInfo {
    QString folderName;
    QString sectorName;     // Raw from folder
    QString resolvedSector; // After mapping
    QString platform;       // Gogo, Net, Tixstock, etc.
    QString orderStatus;    // "Bought with names", "Bought need info", etc.
    int quantity = 0;
    bool isPending = false;
    bool isBought = false;       // "BOUGHT" is present
    bool isStockRequest = false; // Pending but NOT Bought (Needs Stock)
  };

  // Helper to find "caricati" variations
  QString findDeliveredFolder(const QDir &rootDir);

  // New Logic Helpers
  QString classifyPlatform(const QString &folderName);
  QPair<QString, int> parseSectorAndQuantity(const QString &folderName,
                                             const QString &fullPath = "");
  int countPdfs(const QString &path);

  // Mapping & Context
  void loadMappings();
  void saveMappings();
  void loadSectorDB(); // New JSON DB
  QString detectStadiumContext(const QString &folderName);
  QString getCanonicalSectorName(const QString &raw); // Added Declaration

  // Logic from CalcStock
  std::tuple<QString, QString, QString>
  extractSrsFromFilename(const QString &filename);

  QString resolveSector(const QString &folderSectorName,
                        const QStringList &availableSectors,
                        QMap<QString, int> &remainingStock,
                        bool allowInteractive = true);

  // Alias Support
  QString applyStandardAliases(const QString &input);

  struct AliasRule {
    QString pattern;
    QString target;
    bool exact;
  };
  QList<AliasRule> m_aliasRules;
  void initStandardAliases();

  QMap<QString, QString> m_sectorMap; // FolderName -> RealSectorName

  // Sector DB: Context -> (Sector -> [Blocks])
  QMap<QString, QMap<QString, QStringList>> m_sectorDB;
  QString m_currentContext;

  // UI
  QLineEdit *m_pathEdit;
  QPushButton *m_btnBrowse;
  QPushButton *m_btnRun;
  QTextEdit *m_reportArea;
};

} // namespace GOL

#endif // STOCKREPORT_H
