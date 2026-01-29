#include "CheckListing.h"
#include "../CosmicDialog.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include "CalcStock.h" // Added for static logic
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog> // Needed for file dialog
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>

namespace GOL {

CheckListing::CheckListing(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("STOCK ALLOCATOR PRO v2.9 | GOLEVENTS Edition");
  resize(1300, 850);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));
  setupUI();

  // Initial rows
  for (int i = 0; i < 10; ++i)
    addRow();

  connect(&m_calcWatcher, &QFutureWatcher<QString>::finished, this,
          &CheckListing::onCalcFinished);
}

void CheckListing::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // --- Top Bar ---
  QWidget *topBar = new QWidget();
  topBar->setFixedHeight(70);
  topBar->setStyleSheet(QString("background-color: %1;").arg(Utils::CARD_BG));
  QHBoxLayout *topLayout = new QHBoxLayout(topBar);
  topLayout->setContentsMargins(30, 0, 30, 0);

  QLabel *titleLabel = new QLabel("STOCK ALLOCATOR PRO 👑");
  titleLabel->setStyleSheet(
      QString("font-size: 20px; font-weight: bold; color: %1;")
          .arg(Utils::ACCENT_COLOR));
  topLayout->addWidget(titleLabel);

  topLayout->addStretch();

  m_lblGrand = new QLabel("TOTAL LISTED: 0");
  m_lblGrand->setStyleSheet(
      "font-size: 16px; font-weight: bold; color: white;");
  topLayout->addWidget(m_lblGrand);
  mainLayout->addWidget(topBar);

  // --- Main Content ---
  QWidget *contentArea = new QWidget();
  QHBoxLayout *hLayout = new QHBoxLayout(contentArea);
  hLayout->setContentsMargins(20, 20, 20, 20);
  hLayout->setSpacing(20);

  // Left Column
  QVBoxLayout *leftCol = new QVBoxLayout();

  // Section 1 Header with Button
  QHBoxLayout *sec1Layout = new QHBoxLayout();
  sec1Layout->addWidget(createSectionLabel("1. ORIGINAL STOCK INPUT"));
  sec1Layout->addStretch();

  m_btnCalc = new QPushButton("📂 CLICK TO IMPORT STOCK");
  m_btnCalc->setFixedSize(200, 30);
  m_btnCalc->setObjectName("infoButton");
  connect(m_btnCalc, &QPushButton::clicked, this, &CheckListing::onCalcStock);
  sec1Layout->addWidget(m_btnCalc);

  leftCol->addLayout(sec1Layout);

  m_txtOriginal = new QTextEdit();
  m_txtOriginal->setFixedHeight(250);
  m_txtOriginal->setFont(QFont("Consolas", 10));
  // Style for visibility
  m_txtOriginal->setStyleSheet(
      "background-color: #111111; border: 1px solid #444; border-radius: 6px; "
      "padding: 10px; color: #e5e5e5;");

  leftCol->addWidget(m_txtOriginal);

  leftCol->addWidget(createSectionLabel("2. UNIFIED LISTING TABLE"));

  QWidget *tableHeader = new QWidget();
  tableHeader->setObjectName("tableHeader");
  QHBoxLayout *headLayout = new QHBoxLayout(tableHeader);
  // Header Row
  // Header Row
  // Use existing tableHeader and headLayout from lines 97-99 if valid, or clean
  // up here. Looking at the view, lines 97-99 exist. Let's reuse them.
  headLayout->setContentsMargins(15, 5, 15, 5);
  headLayout->setSpacing(5);

  // Lambda for simple Label header
  auto addHeadLabel = [&](const QString &t, int w, const QString &c) {
    QWidget *container = new QWidget();
    container->setFixedWidth(w);
    QHBoxLayout *hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setAlignment(Qt::AlignCenter);

    QLabel *l = new QLabel(t);
    l->setStyleSheet(QString("font-weight: bold; color: %1;").arg(c));
    hLayout->addWidget(l);
    headLayout->addWidget(container);
  };

  // Lambda for Button header
  auto addHeadButton = [&](const QString &t, int w, const QString &c,
                           void (CheckListing::*slot)()) {
    QWidget *container = new QWidget();
    container->setFixedWidth(w);
    QHBoxLayout *hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setAlignment(Qt::AlignCenter);
    hLayout->setSpacing(8);

    QLabel *l = new QLabel(t);
    l->setStyleSheet(QString("font-weight: bold; color: %1;").arg(c));
    hLayout->addWidget(l);

    QPushButton *btn = new QPushButton("📥");
    btn->setFixedSize(28, 24);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setToolTip("Paste from Clipboard");
    btn->setStyleSheet(
        QString("QPushButton { background-color: %1; color: #000; border: "
                "none; border-radius: 4px; font-weight: bold; }"
                "QPushButton:hover { background-color: white; }")
            .arg(c));
    connect(btn, &QPushButton::clicked, this, slot);
    hLayout->addWidget(btn);

    headLayout->addWidget(container);
  };

  // 1. Sector
  addHeadLabel("SECTOR", 100, "gray");

  // 2. Gogo
  addHeadButton("GOGO QTY", 180, "#38bdf8", &CheckListing::onImportGogo);

  // 3. Net
  addHeadButton("NET QTY", 180, "#fb7185", &CheckListing::onImportNet);

  // 4. Tix
  addHeadButton("TIX QTY", 180, "#fbbf24", &CheckListing::onImportTix);

  leftCol->addWidget(tableHeader);

  QScrollArea *scroll = new QScrollArea();
  scroll->setWidgetResizable(true);
  scroll->setStyleSheet(
      "QScrollArea { border: none; background-color: transparent; }");
  QWidget *rowsScrollWidget = new QWidget();
  m_rowsLayout = new QVBoxLayout(rowsScrollWidget);
  m_rowsLayout->setContentsMargins(0, 0, 0, 0);
  m_rowsLayout->setSpacing(5);
  m_rowsLayout->addStretch(); // Important for pinning rows to top
  scroll->setWidget(rowsScrollWidget);
  leftCol->addWidget(scroll);

  QPushButton *btnAdd = new QPushButton("+ ADD NEW SECTOR ROW");
  btnAdd->setFixedHeight(35);
  btnAdd->setObjectName("infoButton");
  connect(btnAdd, &QPushButton::clicked, this, &CheckListing::addRow);
  leftCol->addWidget(btnAdd);

  // Old buttons removed as per request

  hLayout->addLayout(leftCol, 1);

  // Right Column
  QVBoxLayout *rightCol = new QVBoxLayout();
  rightCol->addWidget(createSectionLabel("3. FINAL PROCESSED RESULT"));

  m_txtResult = new QTextEdit();
  m_txtResult->setFont(QFont("Consolas", 12, QFont::Bold));
  // Style for visibility
  m_txtResult->setStyleSheet(
      "background-color: #111111; border: 1px solid #444; border-radius: 6px; "
      "padding: 10px; color: #e5e5e5;");

  rightCol->addWidget(m_txtResult);

  hLayout->addLayout(rightCol, 1);
  mainLayout->addWidget(contentArea);

  // --- Bottom Action Bar ---
  QWidget *actionBar = new QWidget();
  actionBar->setFixedHeight(80);
  QHBoxLayout *actionLayout = new QHBoxLayout(actionBar);
  actionLayout->setContentsMargins(40, 0, 40, 0);

  QPushButton *btnProcess = new QPushButton("🚀  GENERATE FINAL STOCK");
  btnProcess->setFixedHeight(50);
  btnProcess->setFixedWidth(300);
  btnProcess->setCursor(Qt::PointingHandCursor);
  btnProcess->setObjectName(
      "btnGenerate"); // Changed object name to "btnGenerate"
  connect(btnProcess, &QPushButton::clicked, this, &CheckListing::process);
  actionLayout->addWidget(btnProcess);

  QPushButton *btnReset = new QPushButton("🗑️  RESET ALL");
  btnReset->setFixedHeight(50);
  btnReset->setFixedWidth(150);
  btnReset->setObjectName("dangerButton");
  connect(btnReset, &QPushButton::clicked, this, &CheckListing::resetAll);
  actionLayout->addWidget(btnReset);

  actionLayout->addStretch();
  mainLayout->addWidget(actionBar);
}

QWidget *CheckListing::createSectionLabel(const QString &text) {
  QLabel *l = new QLabel(text);
  l->setStyleSheet("font-weight: bold; color: gray; margin-bottom: 5px;");
  return l;
}

void CheckListing::addRow() {
  RowWidgets row;
  row.container = new QWidget();
  QHBoxLayout *l = new QHBoxLayout(row.container);
  l->setContentsMargins(5, 2, 5, 2);
  l->setSpacing(5);

  auto createEdit = [&](int w, const QString &c) {
    QLineEdit *e = new QLineEdit();
    e->setFixedWidth(w);
    e->setFixedHeight(35);
    e->setAlignment(Qt::AlignCenter);
    e->setStyleSheet(QString("background-color: #0d0d0d; color: %1; border: "
                             "none; border-radius: 4px;")
                         .arg(c));
    return e;
  };

  row.sector = createEdit(100, "white");
  row.gogo = createEdit(180, "#38bdf8");
  row.net = createEdit(180, "#fb7185");
  row.tix = createEdit(180, "#fbbf24");

  l->addWidget(row.sector);
  l->addWidget(row.gogo);
  l->addWidget(row.net);
  l->addWidget(row.tix);

  m_unifiedRows.append(row);
  m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row.container);
}

void CheckListing::onImportGogo() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Select Viagogo HTML File", "", "HTML Files (*.html *.htm)");
  if (fileName.isEmpty())
    return;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "Error", "Could not open file.");
    return;
  }

  QString htmlContent = file.readAll();
  file.close();

  // Regex to extract Sector and Available Quantity
  // Sector: <div class="t xs absl t0 w100 ellip">SECTOR</div>
  // Qty: data-edit-field="AvailableTickets">...<span>QTY</span>

  QMap<QString, QString> importedData;

  // We parsed the HTML structure:
  // 1. Find sector div
  // 2. Look ahead for quantity span

  // Revised Strategy (v4):
  // HTML structure analysis reveals:
  // <tr ... data-quantity="8"> ... <div class="t xs absl t0 w100
  // ellip">157</div> ... We use data-quantity as the row anchor, then find the
  // sector immediately following it.

  // 1. Find all rows with quantity
  QRegularExpression rowAnchorRe("data-quantity=\"(\\d+)\"");
  QRegularExpression sectorRe(
      "class=\"t xs absl t0 w100 ellip\">\\s*([^<]+)\\s*</div>");

  auto rowMatch = rowAnchorRe.globalMatch(htmlContent);
  int foundRows = 0;

  while (rowMatch.hasNext()) {
    auto rMatch = rowMatch.next();
    QString qtyStr = rMatch.captured(1);

    // Search for sector immediately after this row definition
    int searchStart = rMatch.capturedEnd();
    auto secMatch = sectorRe.match(htmlContent, searchStart);

    // Sanity check: Sector must be reasonably close (within 1000 chars)
    // and finding it shouldn't jump over another row anchor match (heuristic)
    if (secMatch.hasMatch() &&
        (secMatch.capturedStart() - searchStart < 2000)) {
      QString sector = secMatch.captured(1).trimmed();
      // Clean up sector text if needed (sometimes invisible chars)
      sector.remove(QRegularExpression("[\\n\\r\\t]"));

      if (!sector.isEmpty()) {
        if (importedData.contains(sector))
          importedData[sector] += " + " + qtyStr;
        else
          importedData[sector] = qtyStr;
        foundRows++;
      }
    }
  }

  // Update UI Rows
  int updatedCount = 0;
  for (auto &row : m_unifiedRows) {
    QString currentSector = row.sector->text().trimmed();
    // Try exact match first
    if (importedData.contains(currentSector)) {
      row.gogo->setText(importedData[currentSector]);
      updatedCount++;
    } else {
      // Try case insensitive
      for (auto it = importedData.begin(); it != importedData.end(); ++it) {
        if (it.key().compare(currentSector, Qt::CaseInsensitive) == 0) {
          row.gogo->setText(it.value());
          updatedCount++;
          break;
        }
      }
    }
  }

  if (updatedCount == 0 && foundRows > 0) {
    QMessageBox::warning(
        this, "Partial Match",
        QString("Found %1 rows in HTML, but matched 0 with your list "
                "sectors.\nCheck if your Sector names (e.g. '%2') match the "
                "website.")
            .arg(foundRows)
            .arg(importedData.isEmpty() ? "?" : importedData.firstKey()));
  }

  // Success Message
  if (updatedCount > 0) {
    QMessageBox::information(
        this, "Import Complete",
        QString("Updated %1 rows from Gogo HTML.").arg(updatedCount));
  } else if (foundRows == 0) {
    QMessageBox::warning(this, "Import Failed",
                         "Still could not find any rows. The website format "
                         "might have changed drastically.");
  }
}

void CheckListing::onImportTix() {
  // Show input dialog for user to paste text
  bool ok;
  QString text = QInputDialog::getMultiLineText(
      this, "TixStock Import", "Paste your TixStock listing data here:", "",
      &ok);

  if (!ok || text.trimmed().isEmpty()) {
    return;
  }

  QStringList lines = text.split('\n', Qt::SkipEmptyParts);

  if (lines.isEmpty()) {
    QMessageBox::warning(this, "Warning", "No data provided.");
    return;
  }

  // Parse Tixstock Text Data
  // Pattern based on your example:
  // Line i: "E-ticket" (marker)
  // Line i+1: Quantity (e.g., "2")
  // Line i+2: "0"
  // Line i+3: "All together"
  // Line i+4-5: Numbers
  // Line i+6: Sector name (e.g., "Terzo Anello Verde") - SKIP
  // Line i+7: Block number (e.g., "358") - USE THIS

  QMap<QString, QString> inventory;

  for (int i = 0; i < lines.size(); ++i) {
    QString line = lines[i].trimmed();

    // Look for "E-ticket" marker
    if (line == "E-ticket" && i + 7 < lines.size()) {
      // Quantity is next line
      bool ok;
      int qty = lines[i + 1].trimmed().toInt(&ok);
      if (!ok || qty <= 0)
        continue;

      // Block number is at i+7 (after: qty, 0, "All together", 2 numbers,
      // sector name)
      QString blockNum = lines[i + 7].trimmed();

      // Use block number as the key (like "358", "324", etc.)
      // Skip if it's "Row" or contains currency symbols
      if (!blockNum.isEmpty() && blockNum != "Row" && !blockNum.contains("€") &&
          !blockNum.contains("First")) {
        if (inventory.contains(blockNum)) {
          inventory[blockNum] += " + " + QString::number(qty);
        } else {
          inventory[blockNum] = QString::number(qty);
        }
      }
    }
  }

  if (inventory.isEmpty()) {
    QMessageBox::warning(
        this, "No Data Found",
        QString("Could not parse any listings from the text.\n\nMake sure you "
                "copied the TixStock data correctly."));
    return;
  }

  // Update UI Rows
  int updatedCount = 0;
  for (int r = 0; r < m_unifiedRows.size(); ++r) {
    QString currentSector = m_unifiedRows[r].sector->text().trimmed();
    if (inventory.contains(currentSector)) {
      m_unifiedRows[r].tix->setText(inventory[currentSector]);
      updatedCount++;
    } else {
      // Case insensitive fallback
      for (auto it = inventory.begin(); it != inventory.end(); ++it) {
        if (it.key().compare(currentSector, Qt::CaseInsensitive) == 0) {
          m_unifiedRows[r].tix->setText(it.value());
          updatedCount++;
          break;
        }
      }
    }
  }

  if (updatedCount > 0) {
    QMessageBox::information(
        this, "Import Complete",
        QString("Updated %1 rows from TixStock data.").arg(updatedCount));
  } else {
    QMessageBox::warning(this, "No Matches",
                         QString("Found %1 blocks in clipboard, but matched 0 "
                                 "with your list.\n\nBlocks found: %2")
                             .arg(inventory.size())
                             .arg(inventory.keys().join(", ")));
  }
}

void CheckListing::resetAll() {
  m_txtOriginal->clear();
  m_txtResult->clear();
  for (auto &row : m_unifiedRows) {
    row.sector->clear();
    row.gogo->clear();
    row.net->clear();
    row.tix->clear();
  }
  m_lblGrand->setText("TOTAL LISTED: 0");
}

struct Demand {
  QString platform;
  int qty;
  QString sector;
};

struct RowData {
  QString sector;
  int total;
  int remaining;
  QStringList allocs;
  QString prefix;
};

void CheckListing::process() {
  // Security Check
  SecurityManager::instance().checkAndAct();

  // Visual Feedback
  QPushButton *btnGen = this->findChild<QPushButton *>("btnGenerate");
  if (btnGen) {
    btnGen->setText("⏳ PROCESSING...");
    btnGen->setEnabled(false);
    QApplication::processEvents();
  }

  QString stockText = m_txtOriginal->toPlainText().trimmed();
  if (stockText.isEmpty()) {
    if (btnGen) {
      btnGen->setText("🚀 GENERATE FINAL STOCK");
      btnGen->setEnabled(true);
    }
    return;
  }

  QMap<QString, QMap<QString, QList<int>>> listings; // Platform -> Sector ->
                                                     // List of Qties
  QStringList platforms = {"GOGO", "NET", "TIXSTOCK"};
  int grandSum = 0;

  for (auto &row : m_unifiedRows) {
    if (grandSum % 50 == 0)
      QApplication::processEvents(); // Keep UI alive

    QString sec = row.sector->text().trimmed().toUpper();
    if (sec.isEmpty())
      continue;

    auto parseAndAdd = [&](const QString &plat, QLineEdit *edit) {
      QString val = edit->text().trimmed();
      if (val.isEmpty())
        return;
      QRegularExpression re("\\d+");
      auto it = re.globalMatch(val);
      while (it.hasNext()) {
        int q = it.next().captured(0).toInt();
        listings[plat][sec].append(q);
        grandSum += q;
      }
    };

    parseAndAdd("GOGO", row.gogo);
    parseAndAdd("NET", row.net);
    parseAndAdd("TIXSTOCK", row.tix);
  }

  m_lblGrand->setText(QString("TOTAL LISTED: %1").arg(grandSum));

  // Parse Stock Text
  QRegularExpression rowRe("(.*Row:.*?Qty:\\s*(\\d+)\\s*)\\[(.*?)\\]",
                           QRegularExpression::CaseInsensitiveOption);
  QRegularExpression secRe("Sector:\\s*([A-Za-z0-9]+)",
                           QRegularExpression::CaseInsensitiveOption);

  QStringList lines = stockText.split('\n');
  QMap<int, RowData> rowMap;
  QMap<QString, int> secCap;
  QString currSec;

  for (int i = 0; i < lines.size(); ++i) {
    auto mSec = secRe.match(lines[i]);
    if (mSec.hasMatch()) {
      currSec = mSec.captured(1).toUpper();
      if (!secCap.contains(currSec))
        secCap[currSec] = 0;
    }

    auto mRow = rowRe.match(lines[i]);
    if (mRow.hasMatch() && !currSec.isEmpty()) {
      int qty = mRow.captured(2).toInt();
      secCap[currSec] += qty;
      rowMap[i] = {currSec, qty, qty, {}, mRow.captured(1)};
    }
  }

  // Danger Check
  QStringList danger;
  QSet<QString> allSectors;
  for (const auto &p : platforms)
    for (const auto &s : listings[p].keys())
      allSectors.insert(s);

  for (const QString &s : allSectors) {
    int totNeeded = 0;
    for (const auto &p : platforms)
      for (int q : listings[p][s])
        totNeeded += q;
    int avail = secCap.value(s, 0);
    if (totNeeded > avail) {
      danger.append(QString("Sector %1: %2 listed vs %3 available")
                        .arg(s)
                        .arg(totNeeded)
                        .arg(avail));
    }
  }

  // Danger Check - REMOVED POPUP
  if (!danger.isEmpty()) {
    // Show error inline instead of popup
    m_txtResult->setPlainText("⚠️ STOP! INSUFFICIENT STOCK - PLEASE "
                              "FIX:\n\n" +
                              danger.join('\n'));
    if (this->findChild<QPushButton *>("btnGenerate")) {
      auto btn = this->findChild<QPushButton *>("btnGenerate");
      btn->setText("🚀 GENERATE FINAL STOCK");
      btn->setEnabled(true);
    }
    return;
  }

  // Allocation Logic ... (No changes here, just
  // context if needed, but we scroll down)
  // ...

  // Allocation Logic
  QList<Demand> demands;
  for (const auto &p : platforms) {
    for (auto it = listings[p].begin(); it != listings[p].end(); ++it) {
      for (int q : it.value())
        demands.append({p, q, it.key()});
    }
  }

  // Pass 1: Exact Match
  for (auto &d : demands) {
    for (auto &r : rowMap) {
      if (r.sector == d.sector && r.remaining == d.qty) {
        r.allocs.append(QString("X%1 %2").arg(d.qty).arg(d.platform));
        r.remaining = 0;
        d.qty = 0;
        break;
      }
    }
  }

  // Pass 2: Best Fit (Fits in one row)
  for (auto &d : demands) {
    if (d.qty <= 0)
      continue;
    for (auto &r : rowMap) {
      if (r.sector == d.sector && r.remaining >= d.qty) {
        r.allocs.append(QString("X%1 %2").arg(d.qty).arg(d.platform));
        r.remaining -= d.qty;
        d.qty = 0;
        break;
      }
    }
  }

  // Pass 3: Fractional (Split)
  for (auto &d : demands) {
    if (d.qty <= 0)
      continue;
    for (auto &r : rowMap) {
      if (r.sector == d.sector && r.remaining > 0) {
        int take = qMin(d.qty, r.remaining);
        r.allocs.append(QString("X%1 %2").arg(take).arg(d.platform));
        r.remaining -= take;
        d.qty -= take;
        if (d.qty <= 0)
          break;
      }
    }
  }

  // Final Assembly
  QStringList resultLines;
  QStringList missingStock; // To track under-listing

  for (int i = 0; i < lines.size(); ++i) {
    if (rowMap.contains(i)) {
      RowData &r = rowMap[i];
      if (!r.allocs.isEmpty()) {
        QString allocStr = r.allocs.join(" / ");
        QString stillStr = "";

        if (r.remaining > 0) {
          stillStr = QString(" STILL X%1").arg(r.remaining);
          missingStock.append(
              QString("Sector %1: Missing X%2").arg(r.sector).arg(r.remaining));
        }

        resultLines.append(
            QString("%1[%2]%3").arg(r.prefix, allocStr, stillStr));
      } else {
        // No allocations but we have this row
        // mapped? Means we have stock but 0
        // allocated.
        if (r.total > 0) {
          missingStock.append(QString("Sector %1: 100% Missing (X%2)")
                                  .arg(r.sector)
                                  .arg(r.total));
        }
        resultLines.append(lines[i]);
      }
    } else {
      resultLines.append(lines[i]);
    }
  }

  m_txtResult->setPlainText(resultLines.join('\n'));

  // Restore Button
  if (btnGen) {
    btnGen->setText("🚀 GENERATE FINAL STOCK");
    btnGen->setEnabled(true);
  }

  // MISSING STOCK WARNING - REMOVED POPUP
  if (!missingStock.isEmpty()) {
    QString msg = "\n\n⚠️ WARNING: MISSING ITEMS - YOU HAVE "
                  "NOT LISTED EVERYTHING:\n";
    msg += missingStock.join('\n');
    m_txtResult->append(msg);
  }
}

void CheckListing::onCalcStock() {
  // 1. Select Folder
  QString path = QFileDialog::getExistingDirectory(
      this, "Select Event Folder for Auto-Calculation");
  if (path.isEmpty())
    return;

  bool oddEven = false; // Default

  // 2. Update UI State (Button Only)
  if (m_btnCalc) {
    m_btnCalc->setText("⏳ SCANNING...");
    m_btnCalc->setEnabled(false);
    m_btnCalc->repaint();
    QApplication::processEvents();
  }

  // 3. Start Background Thread
  QFuture<QString> future = QtConcurrent::run([path, oddEven]() {
    return CalcStock::generateReportContent(path, oddEven);
  });

  m_calcWatcher.setFuture(future);
}

void CheckListing::onCalcFinished() {
  // 1. Restore UI State
  if (m_btnCalc) {
    m_btnCalc->setText("⚡ CALCULATE STOCK");
    m_btnCalc->setEnabled(true);
  }

  // 2. Get Result
  QString report = m_calcWatcher.result();

  // 3. Check Error
  if (report == "ERROR_NO_TICKETS_FOLDER") {
    m_txtOriginal->setPlainText("❌ Error: Could not find '- Tickets -' "
                                "folder.\nPlease select the "
                                "main event folder.");
    return;
  }

  // 4. Populate
  m_txtOriginal->setPlainText(report);

  // --- Auto-fill Sectors in Table ---
  QStringList sectors;
  QRegularExpression secRe("Sector:\\s*([^\\s]+)",
                           QRegularExpression::CaseInsensitiveOption);

  QStringList lines = report.split('\n');
  for (const QString &line : lines) {
    if (line.contains("Sector:", Qt::CaseInsensitive)) {
      auto match = secRe.match(line);
      if (match.hasMatch()) {
        sectors.append(match.captured(1).trimmed());
      }
    }
  }

  // Remove duplicate sectors
  sectors.removeDuplicates();

  // Optimize UI updates
  m_rowsLayout->parentWidget()->setUpdatesEnabled(false);

  // Ensure enough rows
  while (m_unifiedRows.size() < sectors.size()) {
    addRow();
    if (m_unifiedRows.size() % 10 == 0) {
      QApplication::processEvents(); // Keep UI responsive
                                     // during heavy creation
    }
  }

  // Fill rows
  for (int i = 0; i < m_unifiedRows.size(); ++i) {
    if (i < sectors.size()) {
      m_unifiedRows[i].sector->setText(sectors[i]);
    } else {
      m_unifiedRows[i].sector->clear();
    }

    if (i % 50 == 0) {
      QApplication::processEvents(); // Keep UI responsive
                                     // during population
    }
  }

  m_rowsLayout->parentWidget()->setUpdatesEnabled(true);

  // Force final repaint
  QApplication::processEvents();
}

void CheckListing::onImportNet() {
  QString fileName =
      QFileDialog::getOpenFileName(this, "Select Football Ticket Net HTML File",
                                   "", "HTML Files (*.html *.htm)");
  if (fileName.isEmpty())
    return;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "Error", "Could not open file.");
    return;
  }

  QString htmlContent = file.readAll();
  file.close();

  QMap<QString, QString> importedData;
  int pos = 0;

  // Strategy: Find class="quantity", extract value,
  // then find data-block in subsequent edit_ticket
  while ((pos = htmlContent.indexOf("class=\"quantity\"", pos)) != -1) {
    // Find quantity value
    int qtyStart = htmlContent.indexOf("class=\"quantity_value\">", pos);
    if (qtyStart == -1) {
      pos += 1;
      continue;
    }
    qtyStart += 23;
    int qtyEnd = htmlContent.indexOf("<", qtyStart);
    if (qtyEnd == -1)
      break;
    QString qty = htmlContent.mid(qtyStart, qtyEnd - qtyStart).trimmed();

    // Find sector (block) in subsequent Action Row
    // -> Edit Ticket data values
    int editStart = htmlContent.indexOf("class=\"edit_ticket", qtyEnd);

    // Safety: Ensure it's reasonably close (e.g.
    // within 2000 chars) to assume same row
    if (editStart != -1 && (editStart - qtyEnd < 2000)) {
      int blockRef = htmlContent.indexOf("data-block=\"", editStart);
      if (blockRef != -1) {
        blockRef += 12;
        int blockEnd = htmlContent.indexOf("\"", blockRef);
        if (blockEnd != -1) {
          QString sector =
              htmlContent.mid(blockRef, blockEnd - blockRef).trimmed();

          if (!sector.isEmpty() && !qty.isEmpty()) {
            if (importedData.contains(sector)) {
              importedData[sector] += " + " + qty;
            } else {
              importedData[sector] = qty;
            }
          }
        }
      }
    }
    pos = qtyEnd;
  }

  int updatedCount = 0;
  for (auto &row : m_unifiedRows) {
    QString currentSector = row.sector->text().trimmed();
    if (importedData.contains(currentSector)) {
      row.net->setText(importedData[currentSector]);
      updatedCount++;
    } else {
      for (auto it = importedData.begin(); it != importedData.end(); ++it) {
        if (it.key().compare(currentSector, Qt::CaseInsensitive) == 0) {
          row.net->setText(it.value());
          updatedCount++;
          break;
        }
      }
    }
  }

  if (updatedCount > 0) {
    QMessageBox::information(
        this, "Import Complete",
        QString("Updated %1 rows from Net HTML.").arg(updatedCount));
  } else {
    QMessageBox::warning(this, "Import Failed",
                         QString("Found %1 blocks in HTML, but matched 0 with "
                                 "your list.\n\nBlocks found: %2")
                             .arg(importedData.size())
                             .arg(importedData.isEmpty()
                                      ? "None"
                                      : importedData.keys().join(", ")));
  }
}

} // namespace GOL
