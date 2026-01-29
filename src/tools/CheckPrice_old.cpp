#include "CheckPrice.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>
#include <algorithm>

namespace GOL {

CheckPrice::CheckPrice(QWidget *parent) : QDialog(parent) {
  SecurityManager::instance().checkAndAct();

  setWindowTitle("TICKET MARKET PARSER PRO 👑");
  resize(900, 800);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(30, 30, 30, 30);
  mainLayout->setSpacing(20);

  // Header
  QLabel *titleLabel = new QLabel("🔍 LISTING CHECKER PRO");
  titleLabel->setStyleSheet(
      QString("font-size: 28px; font-weight: bold; color: %1;")
          .arg(Utils::ACCENT_COLOR));
  mainLayout->addWidget(titleLabel);

  QLabel *subTitle = new QLabel("Compare your listings against market prices");
  subTitle->setStyleSheet("font-size: 14px; color: gray;");
  mainLayout->addWidget(subTitle);

  // Platform Selection
  QHBoxLayout *platformLayout = new QHBoxLayout();
  QLabel *platformLabel = new QLabel("Platform:");
  platformLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
  m_platformSelector = new QComboBox();
  m_platformSelector->addItem("Football Ticket Net (FTN)", (int)Platform::FTN);
  m_platformSelector->addItem("Tixstock", (int)Platform::TIXSTOCK);
  m_platformSelector->addItem("Viagogo", (int)Platform::VIAGOGO);
  m_platformSelector->setStyleSheet(
      QString("background-color: #333; color: white; padding: 8px; "
              "border-radius: 5px; border: 1px solid %1;")
          .arg(Utils::CARD_BORDER));
  platformLayout->addWidget(platformLabel);
  platformLayout->addWidget(m_platformSelector, 1);
  mainLayout->addLayout(platformLayout);

  // File 1 Selection
  QGroupBox *file1Group = new QGroupBox("HTML File 1 (Required)");
  file1Group->setStyleSheet(
      QString("QGroupBox { color: white; border: 1px solid %1; "
              "border-radius: 8px; padding: 15px; margin-top: 10px; }"
              "QGroupBox::title { subcontrol-origin: margin; left: 10px; }")
          .arg(Utils::CARD_BORDER));
  QHBoxLayout *file1Layout = new QHBoxLayout(file1Group);

  m_file1Path = new QLineEdit();
  m_file1Path->setPlaceholderText("Select HTML file...");
  m_file1Path->setReadOnly(true);
  m_file1Path->setStyleSheet("background-color: #333; color: white; padding: "
                             "8px; border-radius: 5px;");

  m_btnBrowse1 = new QPushButton("📁 Browse");
  m_btnBrowse1->setFixedHeight(40);
  m_btnBrowse1->setCursor(Qt::PointingHandCursor);
  m_btnBrowse1->setStyleSheet(
      QString("QPushButton { background-color: %1; color: white; "
              "border-radius: 8px; font-weight: bold; padding: 0 20px; }"
              "QPushButton:hover { background-color: #1a5a8a; }")
          .arg(Utils::ACCENT_COLOR));

  file1Layout->addWidget(m_file1Path);
  file1Layout->addWidget(m_btnBrowse1);
  mainLayout->addWidget(file1Group);

  // File 2 Selection (for Tixstock)
  QGroupBox *file2Group =
      new QGroupBox("HTML File 2 (Tixstock Only - All Listings)");
  file2Group->setStyleSheet(
      QString("QGroupBox { color: white; border: 1px solid %1; "
              "border-radius: 8px; padding: 15px; margin-top: 10px; }"
              "QGroupBox::title { subcontrol-origin: margin; left: 10px; }")
          .arg(Utils::CARD_BORDER));
  QHBoxLayout *file2Layout = new QHBoxLayout(file2Group);

  m_file2Path = new QLineEdit();
  m_file2Path->setPlaceholderText("Select second HTML file (Tixstock only)...");
  m_file2Path->setReadOnly(true);
  m_file2Path->setStyleSheet("background-color: #333; color: white; padding: "
                             "8px; border-radius: 5px;");

  m_btnBrowse2 = new QPushButton("📁 Browse");
  m_btnBrowse2->setFixedHeight(40);
  m_btnBrowse2->setCursor(Qt::PointingHandCursor);
  m_btnBrowse2->setEnabled(false); // Disabled by default
  m_btnBrowse2->setStyleSheet(
      QString("QPushButton { background-color: %1; color: white; "
              "border-radius: 8px; font-weight: bold; padding: 0 20px; }"
              "QPushButton:hover { background-color: #1a5a8a; }"
              "QPushButton:disabled { background-color: #555; }")
          .arg(Utils::ACCENT_COLOR));

  file2Layout->addWidget(m_file2Path);
  file2Layout->addWidget(m_btnBrowse2);
  mainLayout->addWidget(file2Group);

  // Filter Buttons
  QHBoxLayout *filterLayout = new QHBoxLayout();

  m_btnLeftFilter = new QPushButton("⬅️ LEFT FILTER");
  m_btnLeftFilter->setFixedHeight(60);
  m_btnLeftFilter->setCursor(Qt::PointingHandCursor);
  m_btnLeftFilter->setStyleSheet(
      QString("QPushButton { background-color: #8b5cf6; color: white; "
              "border-radius: 12px; font-weight: bold; font-size: 16px; }"
              "QPushButton:hover { background-color: #7c3aed; }"));

  m_btnRightFilter = new QPushButton("RIGHT FILTER ➡️");
  m_btnRightFilter->setFixedHeight(60);
  m_btnRightFilter->setCursor(Qt::PointingHandCursor);
  m_btnRightFilter->setStyleSheet(
      QString("QPushButton { background-color: #ec4899; color: white; "
              "border-radius: 12px; font-weight: bold; font-size: 16px; }"
              "QPushButton:hover { background-color: #db2777; }"));

  filterLayout->addWidget(m_btnLeftFilter);
  filterLayout->addWidget(m_btnRightFilter);
  mainLayout->addLayout(filterLayout);

  // Results area
  m_resultArea = new QTextEdit();
  m_resultArea->setReadOnly(true);
  m_resultArea->setFont(QFont("Consolas", 11));
  m_resultArea->setStyleSheet(
      QString("background-color: #050505; color: #00FF94; border: 1px solid "
              "%1; border-radius: 10px; padding: 10px;")
          .arg(Utils::CARD_BORDER));
  mainLayout->addWidget(m_resultArea);

  // Footer buttons
  QHBoxLayout *footerLayout = new QHBoxLayout();

  m_btnCopy = new QPushButton("📋 COPY REPORT");
  m_btnCopy->setFixedHeight(45);
  m_btnCopy->setCursor(Qt::PointingHandCursor);
  m_btnCopy->setStyleSheet("QPushButton { background-color: #10b981; color: "
                           "black; border-radius: 8px; font-weight: bold; }"
                           "QPushButton:hover { background-color: #059669; }");

  m_btnClear = new QPushButton("🗑️ CLEAR");
  m_btnClear->setFixedHeight(45);
  m_btnClear->setCursor(Qt::PointingHandCursor);
  m_btnClear->setStyleSheet("QPushButton { background-color: #f43f5e; color: "
                            "white; border-radius: 8px; font-weight: bold; }"
                            "QPushButton:hover { background-color: #e11d48; }");

  footerLayout->addWidget(m_btnCopy);
  footerLayout->addWidget(m_btnClear);
  mainLayout->addLayout(footerLayout);

  // Connections
  connect(m_platformSelector,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &CheckPrice::onPlatformChanged);
  connect(m_btnBrowse1, &QPushButton::clicked, this, &CheckPrice::browseFile1);
  connect(m_btnBrowse2, &QPushButton::clicked, this, &CheckPrice::browseFile2);
  connect(m_btnLeftFilter, &QPushButton::clicked, this,
          &CheckPrice::runLeftFilter);
  connect(m_btnRightFilter, &QPushButton::clicked, this,
          &CheckPrice::runRightFilter);
  connect(m_btnCopy, &QPushButton::clicked, this, &CheckPrice::copyToClipboard);
  connect(m_btnClear, &QPushButton::clicked, this, &CheckPrice::clearAll);

  // Initialize state
  m_currentPlatform = Platform::FTN;
  m_currentMode = CompareMode::LEFT_FILTER;
}

void CheckPrice::onPlatformChanged(int index) {
  m_currentPlatform = (Platform)m_platformSelector->currentData().toInt();

  // Enable/disable second file browser for Tixstock
  bool isTixstock = (m_currentPlatform == Platform::TIXSTOCK);
  m_btnBrowse2->setEnabled(isTixstock);
  m_file2Path->setEnabled(isTixstock);
}

void CheckPrice::browseFile1() {
  SecurityManager::instance().checkAndAct();

  QString fileName = QFileDialog::getOpenFileName(this, "Select HTML File", "",
                                                  "HTML Files (*.html)");
  if (!fileName.isEmpty()) {
    m_file1Path->setText(fileName);
  }
}

void CheckPrice::browseFile2() {
  SecurityManager::instance().checkAndAct();

  QString fileName = QFileDialog::getOpenFileName(
      this, "Select Second HTML File (All Listings)", "",
      "HTML Files (*.html)");
  if (!fileName.isEmpty()) {
    m_file2Path->setText(fileName);
  }
}

void CheckPrice::runLeftFilter() {
  SecurityManager::instance().checkAndAct();

  QString file1 = m_file1Path->text();
  if (file1.isEmpty()) {
    QMessageBox::warning(this, "Error", "Please select HTML File 1 first.");
    return;
  }

  // Check if Tixstock needs second file
  if (m_currentPlatform == Platform::TIXSTOCK) {
    QString file2 = m_file2Path->text();
    if (file2.isEmpty()) {
      QMessageBox::warning(
          this, "Error",
          "Tixstock requires both HTML files (My Listings + All Listings).");
      return;
    }
  }

  m_currentMode = CompareMode::LEFT_FILTER;

  // Read files
  QFile f1(file1);
  if (!f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "Error", "Could not open File 1.");
    return;
  }
  QString html1 = QTextStream(&f1).readAll();
  f1.close();

  QString html2;
  if (m_currentPlatform == Platform::TIXSTOCK) {
    QFile f2(m_file2Path->text());
    if (!f2.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QMessageBox::critical(this, "Error", "Could not open File 2.");
      return;
    }
    html2 = QTextStream(&f2).readAll();
    f2.close();
  }

  QString report =
      generateReport(m_currentPlatform, m_currentMode, html1, html2);
  m_resultArea->setText(report);
}

void CheckPrice::runRightFilter() {
  SecurityManager::instance().checkAndAct();

  QString file1 = m_file1Path->text();
  if (file1.isEmpty()) {
    QMessageBox::warning(this, "Error", "Please select HTML File 1 first.");
    return;
  }

  // Check if Tixstock needs second file
  if (m_currentPlatform == Platform::TIXSTOCK) {
    QString file2 = m_file2Path->text();
    if (file2.isEmpty()) {
      QMessageBox::warning(
          this, "Error",
          "Tixstock requires both HTML files (My Listings + All Listings).");
      return;
    }
  }

  m_currentMode = CompareMode::RIGHT_FILTER;

  // Read files
  QFile f1(file1);
  if (!f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "Error", "Could not open File 1.");
    return;
  }
  QString html1 = QTextStream(&f1).readAll();
  f1.close();

  QString html2;
  if (m_currentPlatform == Platform::TIXSTOCK) {
    QFile f2(m_file2Path->text());
    if (!f2.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QMessageBox::critical(this, "Error", "Could not open File 2.");
      return;
    }
    html2 = QTextStream(&f2).readAll();
    f2.close();
  }

  QString report =
      generateReport(m_currentPlatform, m_currentMode, html1, html2);
  m_resultArea->setText(report);
}

void CheckPrice::clearAll() { m_resultArea->clear(); }

void CheckPrice::copyToClipboard() {
  QString text = m_resultArea->toPlainText();
  if (!text.isEmpty()) {
    QApplication::clipboard()->setText(text);
    QMessageBox::information(this, "Success", "Report copied to clipboard!");
  }
}

double CheckPrice::cleanPrice(const QString &priceStr) {
  if (priceStr.isEmpty())
    return 0.0;

  // Remove currency symbols, whitespace, keep digits and decimal separators
  QString clean = priceStr;
  clean.remove(QRegularExpression("[^\\d\\.,]"));

  // Handle European format (comma as decimal separator)
  if (clean.contains(',') && !clean.contains('.')) {
    clean.replace(',', '.');
  }

  bool ok;
  double val = clean.toDouble(&ok);
  return ok ? val : 0.0;
}

QList<Listing> CheckPrice::parseFTNHtml(const QString &html) {
  QList<Listing> listings;

  QRegularExpression rowRegex(
      R"(<div[^>]*class="[^"]*stand_Sprice[^"]*desktop[^"]*"[^>]*>(.*?)</div>\s*</div>\s*</div>)",
      QRegularExpression::DotMatchesEverythingOption);

  auto it = rowRegex.globalMatch(html);

  while (it.hasNext()) {
    auto match = it.next();
    QString rowContent = match.captured(0);

    Listing listing;
    listing.isOwned = rowContent.contains("current_seller_ticket");

    QRegularExpression catRegex(
        R"(<div[^>]*class="[^"]*category[^"]*"[^>]*>(.*?)</div>)");
    auto catMatch = catRegex.match(rowContent);
    if (catMatch.hasMatch()) {
      QString catText = catMatch.captured(1);
      catText.remove(QRegularExpression("<[^>]*>"));
      listing.category = catText.trimmed();
    }

    QRegularExpression blockRegex(R"(Block:\s*([^<\n]+))");
    auto blockMatch = blockRegex.match(rowContent);
    if (blockMatch.hasMatch()) {
      listing.block = blockMatch.captured(1).trimmed();
    }

    QRegularExpression priceRegex(R"(€\s*([\d,\.]+))");
    auto priceMatch = priceRegex.match(rowContent);
    if (priceMatch.hasMatch()) {
      listing.price = cleanPrice(priceMatch.captured(1));
    }

    QRegularExpression qtyRegex(R"(Up To (\d+) Seats)");
    auto qtyMatch = qtyRegex.match(rowContent);
    if (qtyMatch.hasMatch()) {
      listing.quantity = qtyMatch.captured(1).toInt();
    } else {
      listing.quantity = 1;
    }

    if (listing.price > 0) {
      listings.append(listing);
    }
  }

  return listings;
}

QList<Listing> CheckPrice::parseTixstockHtml(const QString &html) {
  QList<Listing> listings;

  QRegularExpression rowRegex(R"(<tr[^>]*>(.*?)</tr>)",
                              QRegularExpression::DotMatchesEverythingOption);
  auto it = rowRegex.globalMatch(html);

  while (it.hasNext()) {
    auto match = it.next();
    QString rowContent = match.captured(1);

    if (rowContent.contains("Ticket Price") ||
        rowContent.contains("Section/Block")) {
      continue;
    }

    QRegularExpression tdRegex(R"(<td[^>]*>(.*?)</td>)",
                               QRegularExpression::DotMatchesEverythingOption);
    auto tdIt = tdRegex.globalMatch(rowContent);

    QStringList cells;
    while (tdIt.hasNext()) {
      QString cell = tdIt.next().captured(1);
      cell.remove(QRegularExpression("<[^>]*>"));
      cells.append(cell.trimmed());
    }

    if (cells.size() < 6)
      continue;

    Listing listing;
    listing.quantity = cells[0].remove(QRegularExpression("\\D")).toInt();
    listing.category = cells[1];
    listing.section = cells[2];
    listing.row = cells[3];
    listing.price = cleanPrice(cells[5]);
    listing.isOwned = false;

    if (listing.price > 0) {
      listings.append(listing);
    }
  }

  return listings;
}

QList<Listing> CheckPrice::parseViagogoHtml(const QString &html) {
  QList<Listing> listings;

  QRegularExpression rowRegex(
      R"(<tr[^>]*class="([^"]*)"[^> ] *>(.*?) < / tr >) ", 
      QRegularExpression::DotMatchesEverythingOption);
  auto it = rowRegex.globalMatch(html);

  while (it.hasNext()) {
    auto match = it.next();
    QString rowClass = match.captured(1);
    QString rowContent = match.captured(2);

    if (rowContent.contains("<th"))
      continue;

    Listing listing;
    listing.isOwned = rowClass.contains("owned");

    QRegularExpression tdRegex(R"(<td[^>]*>(.*?)</td>)",
                               QRegularExpression::DotMatchesEverythingOption);
    auto tdIt = tdRegex.globalMatch(rowContent);

    QStringList cells;
    while (tdIt.hasNext()) {
      QString cell = tdIt.next().captured(1);
      QRegularExpression inputRegex(R"(<input[^>]*value="([^"]*)") ");
          auto inputMatch = inputRegex.match(cell);
      if (inputMatch.hasMatch()) {
        cell = inputMatch.captured(1);
      }
      cell.remove(QRegularExpression("<[^>]*>"));
      cells.append(cell.trimmed());
    }

    if (cells.size() < 5)
      continue;

    listing.section = cells.value(1);
    listing.venueArea = cells.value(2);
    listing.quantity = cells.value(3).remove(QRegularExpression("\\D")).toInt();
    listing.price = cleanPrice(cells.value(4));

    if (listing.price > 0) {
      listings.append(listing);
    }
  }

  return listings;
}

QString CheckPrice::generateReport(Platform platform, CompareMode mode,
                                   const QString &html1, const QString &html2) {
  QList<Listing> owned;
  QList<Listing> market;

  if (platform == Platform::FTN) {
    QList<Listing> all = parseFTNHtml(html1);
    for (const auto &l : all) {
      if (l.isOwned)
        owned.append(l);
      else
        market.append(l);
    }
  } else if (platform == Platform::TIXSTOCK) {
    owned = parseTixstockHtml(html1);
    market = parseTixstockHtml(html2);
  } else if (platform == Platform::VIAGOGO) {
    QList<Listing> all = parseViagogoHtml(html1);
    for (const auto &l : all) {
      if (l.isOwned)
        owned.append(l);
      else
        market.append(l);
    }
  }

  if (owned.isEmpty()) {
    return "No owned listings found. Please check the HTML file.";
  }

  QMap<QString, QList<Listing>> marketMap;
  for (const auto &item : market) {
    QString key;
    if (platform == Platform::FTN) {
      key = (mode == CompareMode::LEFT_FILTER)
                ? QString("%1|%2")
                      .arg(item.category.toLower())
                      .arg(item.block.toLower())
                : item.category.toLower();
    } else if (platform == Platform::TIXSTOCK) {
      key = (mode == CompareMode::LEFT_FILTER)
                ? QString("%1|%2")
                      .arg(item.category.toLower())
                      .arg(item.section.toLower())
                : item.category.toLower();
    } else {
      key = (mode == CompareMode::LEFT_FILTER) ? item.section.toLower()
                                               : item.venueArea.toLower();
    }
    marketMap[key].append(item);
  }

  QStringList reportLines;

  for (const auto &item : owned) {
    QString key;
    if (platform == Platform::FTN) {
      key = (mode == CompareMode::LEFT_FILTER)
                ? QString("%1|%2")
                      .arg(item.category.toLower())
                      .arg(item.block.toLower())
                : item.category.toLower();
    } else if (platform == Platform::TIXSTOCK) {
      key = (mode == CompareMode::LEFT_FILTER)
                ? QString("%1|%2")
                      .arg(item.category.toLower())
                      .arg(item.section.toLower())
                : item.category.toLower();
    } else {
      key = (mode == CompareMode::LEFT_FILTER) ? item.section.toLower()
                                               : item.venueArea.toLower();
    }

    QList<Listing> comps = marketMap.value(key);

    QList<double> validPrices;
    for (const auto &comp : comps) {
      if (comp.quantity == 1 && item.quantity > 1) {
        continue;
      }
      validPrices.append(comp.price);
    }

    double bestComp =
        validPrices.isEmpty()
            ? 0.0
            : *std::min_element(validPrices.begin(), validPrices.end());

    QString status = "*FIRST*";
    if (bestComp > 0 && item.price > bestComp + 0.01) {
      status = "*NOT FIRST*";
    }

    QString block;
    if (platform == Platform::FTN) {
      if (mode == CompareMode::LEFT_FILTER) {
        block += QString("-%1 - block (Filter)\n")
                     .arg(item.block.isEmpty() ? "No Block" : item.block);
        block += QString("-%1 - category\n\n").arg(item.category);
      } else {
        block += QString("-%1 - category (Filter)\n").arg(item.category);
        block += QString("-%1 - block\n\n")
                     .arg(item.block.isEmpty() ? "No Block" : item.block);
      }
    } else if (platform == Platform::TIXSTOCK) {
      if (mode == CompareMode::LEFT_FILTER) {
        block += QString("-%1 - section (Filter)\n").arg(item.section);
        block += QString("-%1 - category\n\n").arg(item.category);
      } else {
        block += QString("-%1 - category (Filter)\n").arg(item.category);
        block += QString("-%1 - section\n\n").arg(item.section);
      }
    } else {
      if (mode == CompareMode::LEFT_FILTER) {
        block += QString("-%1 - section (Filter)\n").arg(item.section);
        block += QString("-%1 - venue area\n\n").arg(item.venueArea);
      } else {
        block += QString("-%1 - venue area (Filter)\n").arg(item.venueArea);
        block += QString("-%1 - section\n\n").arg(item.section);
      }
    }

    block += QString("-Our Price=%.2f €\n").arg(item.price);
    if (bestComp > 0) {
      block += QString("-Comp Price=%.2f €\n").arg(bestComp);
    } else {
      block += "-Comp Price=NO COMPETITORS\n";
    }
    block += QString("%1\n").arg(status);
    block += "---------------------------------";

    reportLines.append(block);
  }

  return reportLines.join("\n");
}

} // namespace GOL
