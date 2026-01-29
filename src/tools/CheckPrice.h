#ifndef CHECKPRICE_H
#define CHECKPRICE_H

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>


namespace GOL {

// Platform types
enum class Platform {
  FTN,      // Football Ticket Net
  TIXSTOCK, // Tixstock
  VIAGOGO,  // Viagogo
  UNKNOWN
};

// Comparison modes
enum class CompareMode {
  LEFT_FILTER, // Compare by Block/Section
  RIGHT_FILTER // Compare by Category/Venue Area
};

// Listing data structure
struct Listing {
  bool isOwned;
  QString category;
  QString block;
  QString section;
  QString row;
  QString venueArea;
  double price;
  int quantity;
};

class CheckPrice : public QDialog {
  Q_OBJECT

public:
  explicit CheckPrice(QWidget *parent = nullptr);

private slots:
  void browseFile1();
  void browseFile2();
  void runLeftFilter();
  void runRightFilter();
  void clearAll();
  void copyToClipboard();
  void onPlatformChanged(int index);

private:
  QString generateReport(Platform platform, CompareMode mode,
                         const QString &html1,
                         const QString &html2 = QString());

  // Platform-specific parsers
  QList<Listing> parseFTNHtml(const QString &html);
  QList<Listing> parseTixstockHtml(const QString &html);
  QList<Listing> parseViagogoHtml(const QString &html);

  // Helper functions
  double cleanPrice(const QString &priceStr);

  // UI Elements
  QComboBox *m_platformSelector;
  QLineEdit *m_file1Path;
  QLineEdit *m_file2Path;
  QPushButton *m_btnBrowse1;
  QPushButton *m_btnBrowse2;
  QPushButton *m_btnLeftFilter;
  QPushButton *m_btnRightFilter;
  QPushButton *m_btnCopy;
  QPushButton *m_btnClear;
  QTextEdit *m_resultArea;

  // State
  Platform m_currentPlatform;
  CompareMode m_currentMode;
};

} // namespace GOL

#endif // CHECKPRICE_H
