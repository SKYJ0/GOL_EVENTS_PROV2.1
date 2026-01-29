#include "StockReport.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QApplication>
#include <QDate>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

namespace GOL {

StockReport::StockReport(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("GOLEVENTS - STOCK & ORDER REPORT PRO 📊");
  resize(600, 500);
  setMinimumSize(300, 300);
  setWindowTitle("GOLEVENTS - STOCK & ORDER REPORT PRO 📊");
  resize(600, 500);
  setMinimumSize(300, 300);
  // Removed hardcoded background

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(30, 30, 30, 30);
  mainLayout->setSpacing(20);

  // Header
  QLabel *title = new QLabel("📊 STOCK INTELLIGENCE");
  title->setObjectName("headerLabel");
  mainLayout->addWidget(title);

  QLabel *sub = new QLabel(
      "Analyze stock vs pending orders. Smart detection and sector mapping.");
  sub->setObjectName("subHeaderLabel");
  mainLayout->addWidget(sub);

  // Folder selection
  QHBoxLayout *pathLayout = new QHBoxLayout();
  m_pathEdit = new QLineEdit();
  m_pathEdit->setPlaceholderText("Select Main Event Folder...");
  m_pathEdit->setReadOnly(true);
  m_pathEdit->setFixedHeight(45);
  // Removed hardcoded LineEdit style

  m_btnBrowse = new QPushButton("BROWSE");
  m_btnBrowse->setFixedSize(120, 45);
  // Removed hardcoded Button style

  pathLayout->addWidget(m_pathEdit);
  pathLayout->addWidget(m_btnBrowse);
  mainLayout->addLayout(pathLayout);

  // Run button
  m_btnRun = new QPushButton("🚀 GENERATE REPORT");
  m_btnRun->setFixedHeight(60);
  m_btnRun->setObjectName("actionButton");
  mainLayout->addWidget(m_btnRun);

  // Report Area
  m_reportArea = new QTextEdit();
  m_reportArea->setReadOnly(true);
  m_reportArea->setFont(QFont("Consolas", 12));
  m_reportArea->setStyleSheet(
      QString("background-color: #050505; color: #00FF88; border: 1px solid "
              "%1; border-radius: 10px; padding: 10px;")
          .arg(Utils::CARD_BORDER));
  mainLayout->addWidget(m_reportArea);

  connect(m_btnBrowse, &QPushButton::clicked, this, &StockReport::browseFolder);
  connect(m_btnRun, &QPushButton::clicked, this, &StockReport::startAnalysis);

  loadMappings();
}

void StockReport::log(const QString &msg) {
  m_reportArea->append(msg);
  QApplication::processEvents(); // Keep UI responsive
}

void StockReport::browseFolder() {
  QString p =
      QFileDialog::getExistingDirectory(this, "Select Main Event Folder");
  if (!p.isEmpty())
    m_pathEdit->setText(p);
}

// ---------------------------------------------------------
// HELPER METHODS (Regex & Logic)
// ---------------------------------------------------------

bool contains(const QString &str, const QString &sub) {
  return str.contains(sub, Qt::CaseInsensitive);
}

QString StockReport::classifyPlatform(const QString &folderName) {
  QString lower = folderName.toLower();
  if (lower.contains("gogo"))
    return "Gogo";
  if (lower.contains("viagogo"))
    return "Gogo";
  if (lower.contains("stubhub"))
    return "StubHub";
  if (lower.contains("ticombo"))
    return "Ticombo";
  if (lower.contains("tixstock"))
    return "Tixstock";
  if (lower.contains("net"))
    return "Net";
  if (lower.contains("sport"))
    return "SportsEvents";

  // FALLBACK: Regex ID Detection
  // Gogo: 9-10 digits (e.g. 626946288)
  if (folderName.contains(QRegularExpression("\\b\\d{9,10}\\b")))
    return "Gogo";
  // Tixstock: 8 char hex (e.g. BBC7F522) - avoiding simple words
  if (folderName.contains(QRegularExpression("\\b[A-F0-9]{8}\\b")))
    return "Tixstock";
  // Net: 7 digits (e.g. 1539879)
  if (folderName.contains(QRegularExpression("\\b\\d{7}\\b")))
    return "Net";

  return "Private/Other";
}

int StockReport::countPdfs(const QString &path) {
  int count = 0;
  QDirIterator it(path, QStringList() << "*.pdf", QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    count++;
  }
  return count;
}

QPair<QString, int>
StockReport::parseSectorAndQuantity(const QString &folderName,
                                    const QString &fullPath) {
  // 1. Quantity
  int qty = 0;
  QRegularExpression qtyRe("x\\s*(\\d+)",
                           QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch qtyMatch = qtyRe.match(folderName);

  if (qtyMatch.hasMatch()) {
    qty = qtyMatch.captured(1).toInt();
  } else if (!fullPath.isEmpty()) {
    qty = countPdfs(fullPath);
  }

  if (qty == 0)
    qty = 1;

  // 2. Sector Extraction
  QString sector = folderName;

  // Priority to IDs: match parts BEFORE the ID
  QRegularExpression gogoRe("^(.*?)\\s+\\d{9}");
  QRegularExpression netRe("^(.*?)\\s+\\d{7}");
  QRegularExpression tixRe("^(.*?)\\s+[A-F0-9]{8}\\b",
                           QRegularExpression::CaseInsensitiveOption);

  QRegularExpressionMatch m;

  if ((m = gogoRe.match(folderName)).hasMatch()) {
    sector = m.captured(1);
  } else if ((m = netRe.match(folderName)).hasMatch()) {
    sector = m.captured(1);
  } else if ((m = tixRe.match(folderName)).hasMatch()) {
    sector = m.captured(1);
  } else {
    // Fallback: part before "xN"
    if (qtyMatch.hasMatch()) {
      int idx = qtyMatch.capturedStart();
      if (idx > 0) {
        sector = folderName.left(idx);
      }
    }
  }

  // Handle "Inside X": if folder says "inside 236", real sector is 236
  QRegularExpression insideRe("inside\\s+([A-Za-z0-9\\s]+)",
                              QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch insideM = insideRe.match(folderName);
  if (insideM.hasMatch()) {
    sector = insideM.captured(1);
  }

  // Cleanup
  sector = sector.trimmed();
  sector.remove(QRegularExpression("Gogo|Net|Tixstock|StubHub|Ticombo",
                                   QRegularExpression::CaseInsensitiveOption));
  sector.remove(QRegularExpression("BOUGHT.*|need info",
                                   QRegularExpression::CaseInsensitiveOption));

  if (sector.endsWith(" ID-", Qt::CaseInsensitive)) {
    sector.chop(4);
  }
  sector = sector.trimmed();
  while (sector.endsWith("-"))
    sector.chop(1);
  while (sector.startsWith("-"))
    sector = sector.mid(1);

  return qMakePair(sector.trimmed(), qty);
}

QString StockReport::findDeliveredFolder(const QDir &rootDir) {
  QStringList candidates = {"caricati", "carricati", "caricatti", "sent",
                            "delivered"};
  QStringList allDirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

  for (const QString &d : allDirs) {
    for (const QString &cand : candidates) {
      if (d.contains(cand, Qt::CaseInsensitive))
        return d;
    }
  }
  return "";
}

// ---------------------------------------------------------
// ANALYSIS (V19 Logic: Regex Update + Extra Fix)
// ---------------------------------------------------------

void StockReport::startAnalysis() {
  SecurityManager::instance().checkAndAct();
  loadSectorDB(); // Ensure DB is loaded

  QString rootPath = m_pathEdit->text();
  if (rootPath.isEmpty() || !QDir(rootPath).exists()) {
    QMessageBox::warning(this, "Error", "Invalid Folder");
    return;
  }

  m_reportArea->clear();

  // Robust Title Extraction: Walk up until we find a non-generic name
  QDir rDir(rootPath);
  QString folderTitle = rDir.dirName();

  while (folderTitle.contains("tickets", Qt::CaseInsensitive) ||
         folderTitle.contains("stock", Qt::CaseInsensitive) ||
         folderTitle.startsWith("-")) {
    if (!rDir.cdUp())
      break; // Stop if root
    folderTitle = rDir.dirName();
  }

  QString dateStr = QDate::currentDate().toString("dd/MM/yyyy");

  // Set Context
  m_currentContext = detectStadiumContext(folderTitle);

  // Header
  log(QString("*Report generated: %1 (v20)*").arg(dateStr));
  log(QString("**Event: %1**").arg(folderTitle));
  if (!m_currentContext.isEmpty()) {
    log(QString("Context: %1").arg(m_currentContext));
  }
  log("");

  // --- 1. Identify Folders ---
  QString deliveredFolderName;
  QString stockFolderName;

  QDir dir(rootPath);
  deliveredFolderName = findDeliveredFolder(dir);

  // Find dedicated Stock Folder (Case Insensitive)
  QStringList allDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QString &d : allDirs) {
    if (d.compare("tickets", Qt::CaseInsensitive) == 0)
      stockFolderName = d;
    else if (d.compare("- Tickets -", Qt::CaseInsensitive) == 0)
      stockFolderName = d;
  }

  // Data Structures
  QMap<QString, int> stockMap;     // Canonical Sector -> Count
  QMap<QString, int> deliveredMap; // Canonical Sector -> Count
  QMap<QString, int> pendingMap;   // Canonical Sector -> Count
  QMap<QString, int> netStockMap;  // Canonical Sector -> Net Count

  // Platform Summary Data
  struct StatBreakdown {
    int qty = 0;
    QMap<QString, int> sectorQty;
  };
  QMap<QString, QMap<QString, StatBreakdown>> platStat;
  QMap<QString, int> platTotals;

  auto addPlatformStat = [&](QString p, QString status, QString sec, int q) {
    if (status.isEmpty())
      status = "(empty folder)";
    if (!status.startsWith("("))
      status = "(" + status + ")";

    platStat[p][status].qty += q;
    platStat[p][status].sectorQty[getCanonicalSectorName(sec)] += q;
    platTotals[p] += q;
  };

  // Regular Expressions for Parsing
  // V19 Regex: Allows [A-Z0-9] in Row/Seat match (e.g., "45B-11-1D.pdf")
  static QRegularExpression strictRe(R"((.+?)-([A-Z0-9]+)-([A-Z0-9]+)\.pdf)",
                                     QRegularExpression::CaseInsensitiveOption);
  // Loose Regex
  static QRegularExpression looseRe(R"(^([A-Z0-9]+).*\.pdf)",
                                    QRegularExpression::CaseInsensitiveOption);

  // --- 2. Process Delivered ---
  if (!deliveredFolderName.isEmpty()) {
    QDir dDir(dir.filePath(deliveredFolderName));
    QDirIterator it(dDir.absolutePath(), QStringList() << "*.pdf", QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
      it.next();
      QString fName = it.fileName();
      QRegularExpressionMatch match = strictRe.match(fName);
      QString sec;
      if (match.hasMatch()) {
        sec = match.captured(1);
      } else {
        sec = QFileInfo(it.filePath()).dir().dirName();
      }
      deliveredMap[getCanonicalSectorName(sec)]++;
    }
  }

  // --- 3. Process Stock Folder (if exists) ---
  if (!stockFolderName.isEmpty()) {
    QDir sDir(dir.filePath(stockFolderName));
    QStringList sEntries = sDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    // 1. Process Folders inside Stock
    for (const QString &se : sEntries) {
      QString fullPath = sDir.filePath(se);

      // NON-Recursive Scan (Match CalcStock)
      QDir subFolder(fullPath);
      QStringList pdfs =
          subFolder.entryList(QStringList() << "*.pdf", QDir::Files);

      int pCount = 0;
      for (const QString &fName : pdfs) {
        pCount++;

        // Use CalcStock logic
        auto srs = extractSrsFromFilename(fName);
        QString cSec = std::get<0>(srs);

        if (!cSec.isEmpty()) {
          stockMap[getCanonicalSectorName(cSec)]++;
        }
      }

      // If empty folder?
      if (pCount == 0) {
        // CalcStock ignores empty folders entirely. Do nothing.
      }
    }

    // 2. Process Loose PDFs in Stock Root (CalcStock Logic)
    QStringList loosePdfs =
        sDir.entryList(QStringList() << "*.pdf", QDir::Files);
    for (const QString &f : loosePdfs) {
      auto srs = extractSrsFromFilename(f);
      QString cSec = std::get<0>(srs);
      if (!cSec.isEmpty()) {
        stockMap[getCanonicalSectorName(cSec)]++;
      }
    }
  }

  // --- 4. Process Root Folders & Root PDFs ---
  for (const QString &e : allDirs) {
    if (!deliveredFolderName.isEmpty() && e == deliveredFolderName)
      continue;
    if (!stockFolderName.isEmpty() && e == stockFolderName)
      continue;
    if (e.contains("IGNORE", Qt::CaseInsensitive))
      continue;

    QString fullPath = dir.filePath(e);

    // Strict Stock: Starts or Ends with hyphen
    bool isStock = (e.startsWith("-") || e.endsWith("-")) &&
                   !e.contains("BOUGHT", Qt::CaseInsensitive);

    // FIX: If we found a main Stock Folder (e.g. "- Tickets -"), IGNORE other
    // "stock-like" folders in root to strictly match CalcStock which only looks
    // inside "- Tickets -".
    if (!stockFolderName.isEmpty() && isStock) {
      continue;
    }

    if (isStock) {
      // Root Stock Folder (Only if no main stock folder found)
      QDirIterator it(fullPath, QStringList() << "*.pdf", QDir::Files,
                      QDirIterator::Subdirectories);
      int pCount = 0;
      while (it.hasNext()) {
        it.next();
        pCount++;
        QString fName = it.fileName();

        // Use CalcStock logic
        auto [cSec, cRow, cSeat] = extractSrsFromFilename(fName);

        if (!cSec.isEmpty()) {
          stockMap[getCanonicalSectorName(cSec)]++;
        }
      }

    } else {
      // Pending Order
      QString status;
      if (contains(e, "BOUGHT WITH NAMES"))
        status = "Bought with names";
      else if (contains(e, "need info"))
        status = "Bought need info";
      else if (contains(e, "BOUGHT"))
        status = "Bought";

      int pdfCount = countPdfs(fullPath);
      QPair<QString, int> info = parseSectorAndQuantity(e, fullPath);
      QString sec = info.first;
      int qty = (pdfCount > 0) ? pdfCount : info.second;

      pendingMap[getCanonicalSectorName(sec)] += qty;

      QString platform = classifyPlatform(e);
      addPlatformStat(platform, status, sec, qty);
    }
  }

  // --- CALC NET STOCK & PREPARE DEBUG INFO ---
  netStockMap = stockMap;
  QMap<QString, QStringList> debugSubtractionList;

  QMap<QString, int> stockRequests;
  for (const QString &e : allDirs) {
    if (!deliveredFolderName.isEmpty() && e == deliveredFolderName)
      continue;
    if (!stockFolderName.isEmpty() && e == stockFolderName)
      continue;
    if (e.contains("IGNORE", Qt::CaseInsensitive))
      continue;

    bool isStock = (e.startsWith("-") || e.endsWith("-")) &&
                   !e.contains("BOUGHT", Qt::CaseInsensitive);
    if (isStock)
      continue;

    bool isBought = contains(e, "BOUGHT");
    if (isBought)
      continue;

    QString fullPath = dir.filePath(e);
    if (countPdfs(fullPath) > 0)
      continue;

    QPair<QString, int> info = parseSectorAndQuantity(e, fullPath);
    QString sec = info.first;
    int qty = info.second;
    QString canon = getCanonicalSectorName(sec);

    stockRequests[canon] += qty;
    debugSubtractionList[canon].append(QString("%1 (x%2)").arg(e).arg(qty));
  }

  for (auto it = stockRequests.begin(); it != stockRequests.end(); ++it) {
    netStockMap[it.key()] -= it.value();
  }

  // --- OUTPUT GENERATION ---

  auto printSection = [&](const QString &title, QMap<QString, int> &map,
                          int &total) {
    log(QString("*%1*:").arg(title));
    if (title != "Stock")
      log("-------------------------");

    total = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
      if (it.value() != 0) {
        if (title == "Stock") {
          int phys = stockMap[it.key()];
          int pend = stockRequests[it.key()];
          if (pend > 0) {
            log(QString("%1: %2 (%3 Phys - %4 Requests)")
                    .arg(it.key())
                    .arg(it.value())
                    .arg(phys)
                    .arg(pend));
          } else {
            log(QString("%1: %2").arg(it.key()).arg(it.value()));
          }
        } else {
          log(QString("%1: %2").arg(it.key()).arg(it.value()));
        }
        total += it.value();
      }
    }
    log("-------------------------");
    log(QString("*TOTAL %1*        : %2").arg(title).arg(total));
    log("");
  };

  int tStock = 0;
  printSection("Stock", netStockMap, tStock);

  int tPend = 0;
  printSection("Pending", pendingMap, tPend);

  int tDeliv = 0;
  printSection("Delivered", deliveredMap, tDeliv);

  log("*Platform Sales (Summary)*");
  log("-------------------------");
  int grandSold = 0;
  QStringList plats = platTotals.keys();
  plats.sort();

  for (const QString &p : plats) {
    QStringList parts;
    QMap<QString, StatBreakdown> &stats = platStat[p];
    for (auto sit = stats.begin(); sit != stats.end(); ++sit) {
      QString status = sit.key();
      int qty = sit.value().qty;

      QStringList secParts;
      QMap<QString, int> &smap = sit.value().sectorQty;
      for (auto kit = smap.begin(); kit != smap.end(); ++kit) {
        secParts << QString("x%1 %2").arg(kit.value()).arg(kit.key());
      }
      parts << QString("%1 %2 (%3)")
                   .arg(status)
                   .arg(qty)
                   .arg(secParts.join(" + "));
    }

    if (!parts.isEmpty()) {
      log(QString("%1        : %2").arg(p, -10).arg(parts[0]));
      for (int i = 1; i < parts.size(); ++i) {
        log(QString("              %1").arg(parts[i]));
      }
    }
    grandSold += platTotals[p];
  }
  log("-------------------------");
  log(QString("*TOTAL SOLD*          : %1").arg(grandSold));
  log("");

  log("*Stock Deduction Logic (Debug)*:");
  log("---------------------------");
  bool anyDebug = false;
  for (auto it = debugSubtractionList.begin(); it != debugSubtractionList.end();
       ++it) {
    if (!it.value().isEmpty()) {
      log(QString("Sector %1 deducted folders:").arg(it.key()));
      for (const QString &f : it.value()) {
        log(QString(" - %1").arg(f));
      }
      anyDebug = true;
    }
  }
  if (!anyDebug)
    log("No pending requests subtracted.");
  log("---------------------------");

  for (auto it = netStockMap.begin(); it != netStockMap.end(); ++it) {
    if (it.value() < 0) {
      log(QString("WARNING: Negative Stock for %1 (%2). Check mappings or "
                  "missing stock tickets.")
              .arg(it.key())
              .arg(it.value()));
    }
  }

  QFile file(dir.filePath("report.txt"));
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << m_reportArea->toPlainText();
  }
}

// ---------------------------------------------------------
// SECTOR DB & MAPPING
// ---------------------------------------------------------

void StockReport::loadSectorDB() {
  QString path = "resources/sector_db.json";
  if (!QFile::exists(path)) {
    path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           "/sector_db.json";
  }

  QFile file(path);
  if (file.open(QIODevice::ReadOnly)) {
    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = root.begin(); it != root.end(); ++it) {
      QString context = it.key().toLower();
      QJsonObject sectors = it.value().toObject();
      for (auto sit = sectors.begin(); sit != sectors.end(); ++sit) {
        QString realSector = sit.key();
        QJsonArray blocks = sit.value().toArray();
        QStringList blockList;
        for (const auto &v : blocks)
          blockList << v.toString();
        m_sectorDB[context][realSector] = blockList;
      }
    }
  }
}

QString StockReport::detectStadiumContext(const QString &folderName) {
  QString lower = folderName.toLower();
  if (lower.contains("roma") || lower.contains("lazio"))
    return "roma";
  if (lower.contains("inter") || lower.contains("milan"))
    return "inter";
  if (lower.contains("fiorentina"))
    return "fiorentina";
  if (lower.contains("bologna"))
    return "bologna";
  if (lower.contains("atalanta"))
    return "atalanta";
  return "";
}

QString StockReport::getCanonicalSectorName(const QString &raw) {
  QString norm = raw.toLower().trimmed();

  // Clean "sector"
  if (norm.startsWith("sector "))
    norm = norm.mid(7).trimmed();

  // --- SAN SIRO NUMERIC MAPPING (Primary Check) ---
  if (m_currentContext == "inter" || m_currentContext == "milan") {
    bool ok;
    int n = norm.toInt(&ok);
    if (ok) {
      // RED (ROSSO)
      if ((n >= 26 && n <= 36) || (n >= 170 && n <= 172))
        return "PRIMO ROSSO";
      if (n >= 221 && n <= 238)
        return "SECONDO ROSSO";
      if (n >= 319 && n <= 342)
        return "TERZO ROSSO";

      // BLUE (BLU)
      if (n >= 101 && n <= 112)
        return "PRIMO BLU";
      if (n >= 201 && n <= 218)
        return "SECONDO BLU";
      if (n >= 301 && n <= 318)
        return "TERZO BLU";

      // GREEN (VERDE)
      if (n >= 137 && n <= 148)
        return "PRIMO VERDE";
      if (n >= 239 && n <= 254)
        return "SECONDO VERDE";
      if (n >= 343 && n <= 360)
        return "TERZO VERDE";

      // ORANGE (ARANCIO)
      if (n >= 149 && n <= 172)
        return "PRIMO ARANCIO";
      if (n >= 255 && n <= 276)
        return "SECONDO ARANCIO";
    }
  }

  // V18 Rule: EXTRA -> CURVA
  if (norm == "extra")
    return "CURVA";

  if (!m_currentContext.isEmpty() && m_sectorDB.contains(m_currentContext)) {
    QMap<QString, QStringList> &contextRules = m_sectorDB[m_currentContext];
    for (auto it = contextRules.begin(); it != contextRules.end(); ++it) {
      QString sector = it.key();
      QStringList blocks = it.value();

      for (const QString &b : blocks) {
        QRegularExpression re(
            QString("\\b%1\\b").arg(QRegularExpression::escape(b)),
            QRegularExpression::CaseInsensitiveOption);
        if (re.match(raw).hasMatch()) {
          return getCanonicalSectorName(sector);
        }
      }
    }
  }

  if (contains(norm, "long side lower") ||
      (contains(norm, "primo") && contains(norm, "rosso")) ||
      (contains(norm, "primo") && contains(norm, "arancio")) ||
      contains(norm, "1 tier central")) {
    return "1st TIER CENTRAL";
  }

  if (contains(norm, "terzo rosso") || contains(norm, "terzo anello rosso") ||
      contains(norm, "long side upper") || contains(norm, "long side") ||
      contains(norm, "category 1") || contains(norm, "anello rosso")) {
    return "TERZO ROSSO";
  }

  if (contains(norm, "curva") || contains(norm, "short side")) {
    return "CURVA";
  }

  if (contains(norm, "distinti") || contains(norm, "corner area")) {
    return "DISTINTI";
  }

  if ((contains(norm, "tevere") && contains(norm, "central")) ||
      contains(norm, "tevere central stand")) {
    return "TRIBUNA TEVERE CENTRAL";
  }

  if (contains(norm, "monte mario")) {
    return "MONTE MARIO TOP STAND";
  }

  return raw.toUpper();
}

void StockReport::loadMappings() {
  QString path =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/sector_map.json";
  QFile file(path);
  if (file.open(QIODevice::ReadOnly)) {
    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
      m_sectorMap[it.key()] = it.value().toString();
    }
  }
}

void StockReport::saveMappings() {
  QString path =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/sector_map.json";
  QJsonObject obj;
  for (auto it = m_sectorMap.begin(); it != m_sectorMap.end(); ++it) {
    obj[it.key()] = it.value();
  }
  QFile file(path);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(QJsonDocument(obj).toJson());
  }
}

QString StockReport::resolveSector(const QString &folderSectorName,
                                   const QStringList &availableSectors,
                                   QMap<QString, int> &remainingStock,
                                   bool allowInteractive) {
  return folderSectorName;
}

std::tuple<QString, QString, QString>
StockReport::extractSrsFromFilename(const QString &filename) {
  // Logic from python: remove -FV..., split by [- ], take first 3 parts
  QString cleanName = QFileInfo(filename).baseName();

  // Remove -FV... part
  QRegularExpression fvRe("-FV.*", QRegularExpression::CaseInsensitiveOption);
  cleanName.remove(fvRe);

  // Split by - or space
  QStringList parts =
      cleanName.split(QRegularExpression("[- ]"), Qt::SkipEmptyParts);
  if (parts.size() >= 3) {
    return {parts[0].toUpper(), parts[1], parts[2]};
  }
  return {};
}

} // namespace GOL
