#include "VerifyOrders.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <OpenXLSX.hpp>
#include <QDateTime>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>
#include <algorithm>
#include <vector>


namespace GOL {

VerifyOrders::VerifyOrders(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("GOLEVENTS - STRICT VERIFIER PRO 🔍");
  resize(950, 850);
  // Removed hardcoded background

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(40, 30, 40, 40);
  mainLayout->setSpacing(20);

  // Header
  QLabel *title = new QLabel("🔍 STRICT ORDER VERIFIER");
  title->setObjectName("headerLabel");
  mainLayout->addWidget(title);

  QLabel *sub =
      new QLabel("Targeted scan for missing or duplicated Order IDs.");
  sub->setObjectName("subHeaderLabel");
  mainLayout->addWidget(sub);

  // Folder selection
  QHBoxLayout *pathLayout = new QHBoxLayout();
  m_pathEdit = new QLineEdit();
  m_pathEdit->setPlaceholderText("Select match folder to scan...");
  m_pathEdit->setReadOnly(true);
  m_pathEdit->setFixedHeight(45);
  // Removed QLineEdit style

  m_btnBrowse = new QPushButton("BROWSE FOLDER");
  m_btnBrowse->setFixedSize(140, 45);
  m_btnBrowse->setObjectName("infoButton");

  pathLayout->addWidget(m_pathEdit);
  pathLayout->addWidget(m_btnBrowse);
  mainLayout->addLayout(pathLayout);

  // Sales files selection
  m_btnSales = new QPushButton("📂 SELECT SALES FILES (CSV)");
  m_btnSales->setFixedHeight(50);
  m_btnSales->setObjectName("infoButton");
  mainLayout->addWidget(m_btnSales);

  // Run button
  m_btnRun = new QPushButton("🚀 START TARGETED SCAN");
  m_btnRun->setFixedHeight(60);
  m_btnRun->setObjectName("actionButton");
  mainLayout->addWidget(m_btnRun);

  // Log area
  m_logArea = new QTextEdit();
  m_logArea->setReadOnly(true);
  m_logArea->setFont(QFont("Consolas", 12));
  m_logArea->setStyleSheet(
      QString("background-color: #050505; color: #00FF94; border: 1px solid "
              "%1; border-radius: 10px; padding: 10px;")
          .arg(Utils::CARD_BORDER));
  mainLayout->addWidget(m_logArea);

  // Connections
  connect(m_btnBrowse, &QPushButton::clicked, this,
          &VerifyOrders::browseFolder);
  connect(m_btnSales, &QPushButton::clicked, this,
          &VerifyOrders::selectSalesFiles);
  connect(m_btnRun, &QPushButton::clicked, this, &VerifyOrders::startScan);
}

void VerifyOrders::log(const QString &msg) { m_logArea->append("> " + msg); }

void VerifyOrders::browseFolder() {
  QString p = QFileDialog::getExistingDirectory(this, "Select scan folder");
  if (!p.isEmpty())
    m_pathEdit->setText(p);
}

// Helper for parsing CSV properly (handling quotes)
QStringList parseCsvLine(const QString &line, QChar delim) {
  QStringList list;
  QString current;
  bool inQuote = false;
  for (int i = 0; i < line.length(); ++i) {
    QChar c = line[i];
    if (c == '"') {
      if (i + 1 < line.length() && line[i + 1] == '"') {
        current += '"';
        i++; // Skip escaped quote
      } else {
        inQuote = !inQuote;
      }
    } else if (c == delim && !inQuote) {
      list.append(current.trimmed());
      current.clear();
    } else {
      current += c;
    }
  }
  list.append(current.trimmed());
  return list;
}

void VerifyOrders::selectSalesFiles() {
  // Allow users to see all files so we can catch Excel and warn them
  QStringList files = QFileDialog::getOpenFileNames(
      this, "Select Sales Data", "",
      "Data Files (*.csv *.xlsx);;CSV Files (*.csv);;Excel Files (*.xlsx)");

  QStringList validFiles;
  bool excelWarned = false;

  for (const QString &f : files) {
    if (f.endsWith(".xls", Qt::CaseInsensitive)) {
      if (!excelWarned) {
        QMessageBox::warning(this, "Legacy Excel Not Supported",
                             "The old .xls format is not supported. Please "
                             "save as .xlsx or .csv");
        excelWarned = true;
      }
    } else {
      validFiles.append(f);
    }
  }

  if (!validFiles.isEmpty()) {
    m_salesFiles = validFiles;
    log(QString("%1 valid CSV files selected").arg(validFiles.size()));
  }
}

// Helper to clean CSV headers
QString VerifyOrders::cleanHeader(const QString &header) {
  QString h = header.trimmed().replace("\"", "").toLower();
  // Remove leading non-alphanumeric characters (Python behavior)
  int i = 0;
  while (i < h.length() && !h[i].isLetterOrNumber()) {
    i++;
  }
  return h.mid(i);
}

QStringList VerifyOrders::extractFromCsv(const QString &filePath) {
  QStringList extracted;
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return extracted;

  QTextStream in(&file);
  QString headerLine = in.readLine();
  if (headerLine.isNull())
    return extracted;

  QChar delim = headerLine.contains('\t') ? '\t' : ',';
  // Use robust parser for headers too
  QStringList headers = parseCsvLine(headerLine, delim);

  QList<int> idIndices;
  for (int i = 0; i < headers.size(); ++i) {
    QString cleaned = cleanHeader(headers[i]);
    // Utils::logToFile("Header: " + headers[i] + " -> " + cleaned); // Debug
    if (ALLOWED_COLUMNS.contains(cleaned)) {
      idIndices.append(i);
    }
  }

  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.trimmed().isEmpty())
      continue;

    QStringList fields = parseCsvLine(line, delim);
    for (int idx : idIndices) {
      if (idx < fields.size()) {
        QString val = fields[idx];
        // Remove internal quotes if any left (usually handled by parser, but
        // just in case)
        val.remove('"');
        if (val.length() >= 5)
          extracted.append(val);
      }
    }
  }
  return extracted;
}

QStringList VerifyOrders::extractFromExcel(const QString &filePath) {
  QStringList extracted;
  try {
    OpenXLSX::XLDocument doc;
    doc.open(filePath.toStdString());

    auto sheetNames = doc.workbook().worksheetNames();
    if (sheetNames.empty()) {
      log("❌ Excel Error: No sheets found in " +
          QFileInfo(filePath).fileName());
      return extracted;
    }

    // Use the first sheet found
    std::string firstSheet = sheetNames[0];
    auto wks = doc.workbook().worksheet(firstSheet);

    // Scan first 5 rows for header
    int headerRow = -1;
    std::vector<int> idIndices;
    int colCount = wks.columnCount();
    int rowCount = wks.rowCount();

    for (int r = 1; r <= std::min(5, rowCount); ++r) {
      bool foundAny = false;
      for (int c = 1; c <= colCount; ++c) {
        OpenXLSX::XLCell cell = wks.cell(r, c);
        if (cell.value().type() == OpenXLSX::XLValueType::String) {
          QString val = QString::fromStdString(cell.value().get<std::string>());
          if (ALLOWED_COLUMNS.contains(cleanHeader(val))) {
            idIndices.push_back(c);
            foundAny = true;
          }
        }
      }
      if (foundAny) {
        headerRow = r;
        break;
      }
    }

    if (headerRow == -1) {
      log("⚠️ No valid specific header ('id', 'order id'...) found in first 5 "
          "rows of " +
          QFileInfo(filePath).fileName() + ". Checking all columns...");
      // If no header found, maybe try to scan all columns for data looking like
      // IDs? For now, let's just default to scanning all columns if user didn't
      // provide headers, OR warn user. Let's stick to requiring headers for
      // safety, but maybe log valid headers found to help debug?
      return extracted;
    }

    // Iterate Rows starting after header
    for (int r = headerRow + 1; r <= rowCount; ++r) {
      for (int c : idIndices) {
        OpenXLSX::XLCell cell = wks.cell(r, c);
        QString val;
        // Handle types
        if (cell.value().type() == OpenXLSX::XLValueType::String)
          val = QString::fromStdString(cell.value().get<std::string>());
        else if (cell.value().type() == OpenXLSX::XLValueType::Integer)
          val = QString::number(cell.value().get<int64_t>());
        else if (cell.value().type() == OpenXLSX::XLValueType::Float)
          val = QString::number(
              (int64_t)cell.value()
                  .get<double>()); // Handle float as int if it looks like ID

        val = val.trimmed();
        if (val.length() >= 5)
          extracted.append(val);
      }
    }
    doc.close();

  } catch (const std::exception &e) {
    log("❌ Excel Error: " + QString::fromStdString(e.what()));
  }
  return extracted;
}

void VerifyOrders::startScan() {
  // Security Check
  SecurityManager::instance().checkAndAct();

  if (m_pathEdit->text().isEmpty() || m_salesFiles.isEmpty()) {
    QMessageBox::warning(this, "Missing Info",
                         "Please select scan folder and sales files.");
    return;
  }

  m_logArea->clear();
  log("Extracting IDs from sales files...");

  QStringList allSalesIds;
  for (const QString &f : m_salesFiles) {
    QStringList ids;
    if (f.endsWith(".xlsx", Qt::CaseInsensitive)) {
      ids = extractFromExcel(f);
    } else {
      ids = extractFromCsv(f);
    }
    allSalesIds.append(ids);
    log(QString("📄 %1: Found %2 IDs")
            .arg(QFileInfo(f).fileName())
            .arg(ids.size()));
  }

  allSalesIds.removeDuplicates();
  log(QString("Total unique IDs to check: %1").arg(allSalesIds.size()));

  log("Scanning local directories...");
  QMap<QString, QStringList> idMatches;

  QDirIterator it(m_pathEdit->text(), QDir::Dirs | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  QStringList allLocalFolders;
  while (it.hasNext()) {
    allLocalFolders.append(it.next());
  }

  for (const QString &s_id : allSalesIds) {
    QRegularExpression re("\\b" + QRegularExpression::escape(s_id) + "\\b",
                          QRegularExpression::CaseInsensitiveOption);
    for (const QString &folderPath : allLocalFolders) {
      QString folderName = QFileInfo(folderPath).fileName();
      if (re.match(folderName).hasMatch()) {
        idMatches[s_id].append(folderName);
      }
    }
  }

  QStringList missing;
  QMap<QString, QStringList> dupes;
  for (const QString &sid : allSalesIds) {
    if (!idMatches.contains(sid) || idMatches[sid].isEmpty()) {
      missing.append(sid);
    } else if (idMatches[sid].size() > 1) {
      dupes[sid] = idMatches[sid];
    }
  }

  log("\n" + QString(40, '='));
  log(QString("✅ PERFECT MATCHES : %1").arg(idMatches.size() - dupes.size()));
  log(QString("❌ MISSING IDs     : %1").arg(missing.size()));
  log(QString("🚨 DUPLICATED IDs  : %1").arg(dupes.size()));
  log(QString(40, '='));

  if (!missing.isEmpty()) {
    log("\n❌ MISSING ORDERS LIST:");
    for (const QString &miss : missing) {
      log("   - " + miss);
    }
  }

  if (!dupes.isEmpty()) {
    log("\n🚨 DUPLICATED ORDERS LIST:");
    for (auto itD = dupes.begin(); itD != dupes.end(); ++itD) {
      log(QString("   - %1 found in: [%2]")
              .arg(itD.key())
              .arg(itD.value().join(", ")));
    }
  }

  if (missing.isEmpty() && dupes.isEmpty()) {
    QMessageBox::information(this, "Success",
                             "Perfect match! No missing or duplicated IDs.");
  }
}

} // namespace GOL
