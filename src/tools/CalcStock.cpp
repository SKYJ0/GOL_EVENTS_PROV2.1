#include "CalcStock.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>

namespace GOL {

CalcStock::CalcStock(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("Stock Calculator - GOLEVENTS");
  resize(1100, 900);
  setStyleSheet(QString("background-color: #0A0C12; color: white;"));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QLabel *title = new QLabel("📊 STOCK CALCULATOR PRO");
  title->setStyleSheet("color: #00D1FF; font-size: 32px; font-weight: bold;");
  title->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(title);
  mainLayout->addSpacing(30);

  // Path selection
  QHBoxLayout *pathLayout = new QHBoxLayout();
  m_pathInput = new QLineEdit();
  m_pathInput->setPlaceholderText("Select event folder...");
  m_pathInput->setStyleSheet(
      "QLineEdit { background-color: #1A1D29; color: white; border: 1px solid "
      "#333; "
      "border-radius: 8px; padding: 12px; font-size: 13px; }");
  pathLayout->addWidget(m_pathInput, 1);

  QPushButton *browseBtn = new QPushButton("BROWSE EVENT");
  browseBtn->setFixedSize(150, 45);
  browseBtn->setStyleSheet(
      "QPushButton { background-color: #00D1FF; color: black; border: none; "
      "border-radius: 8px; font-size: 13px; font-weight: bold; }"
      "QPushButton:hover { background-color: #00b8e6; }");
  connect(browseBtn, &QPushButton::clicked, this, &CalcStock::browsePath);
  pathLayout->addWidget(browseBtn);

  mainLayout->addLayout(pathLayout);

  m_oddEvenMode = new QCheckBox("Odd-Even Mode (ex: Fiorentina events)");
  m_oddEvenMode->setStyleSheet("font-size: 15px; color: #AAB;");
  mainLayout->addWidget(m_oddEvenMode);
  mainLayout->addSpacing(15);

  QPushButton *generateBtn = new QPushButton("🚀 GENERATE & SAVE REPORT");
  generateBtn->setFixedHeight(60);
  generateBtn->setStyleSheet(
      "QPushButton { background-color: #00FF94; color: black; border: none; "
      "border-radius: 12px; font-size: 18px; font-weight: bold; }"
      "QPushButton:hover { background-color: #00e684; }");
  connect(generateBtn, &QPushButton::clicked, this, &CalcStock::runCalculation);
  mainLayout->addWidget(generateBtn);
  mainLayout->addSpacing(15);

  m_logArea = new QTextEdit();
  m_logArea->setReadOnly(true);
  m_logArea->setStyleSheet("QTextEdit { background-color: #05070B; color: "
                           "#00FF94; border: 1px solid #1A1D29; "
                           "border-radius: 8px; padding: 10px; font-family: "
                           "Consolas; font-size: 14px; }");
  mainLayout->addWidget(m_logArea);
}

void CalcStock::browsePath() {
  QString path = QFileDialog::getExistingDirectory(this, "Select Event Folder");
  if (!path.isEmpty()) {
    m_pathInput->setText(path);
  }
}

void CalcStock::log(const QString &msg, bool clear) {
  if (clear)
    m_logArea->clear();
  m_logArea->append(msg);
  // Force UI update
  QCoreApplication::processEvents();
}

// ---------------------------------------------------------
// STATIC LOGIC
// ---------------------------------------------------------

std::tuple<QString, QString, QString>
CalcStock::extractSrsFromFilename(const QString &filename) {
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

QString CalcStock::extractFvFromFilename(const QString &filename) {
  QRegularExpression re("FV(\\d+(?:p\\d+)?)(?![0-9])",
                        QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch match = re.match(filename);
  if (match.hasMatch()) {
    QString val = match.captured(1);
    val.replace('p', '.');
    return val + "€";
  }
  return "N/A";
}

std::pair<int, QString> CalcStock::parseSeatDetailed(const QString &seatStr) {
  // Remove "ticket", ".pdf", key chars
  QString clean = seatStr;
  clean.remove(QRegularExpression("(?i)ticket|\\.pdf|[^\\d[A-Za-z]]"));

  QRegularExpression re("(\\d+)([A-Za-z]*)");
  QRegularExpressionMatch match = re.match(clean);

  if (match.hasMatch()) {
    return {match.captured(1).toInt(), match.captured(2)};
  }
  return {0, seatStr};
}

QList<QList<CalcStock::SeatInfo>>
CalcStock::findConsecutiveGroups(QList<CalcStock::SeatInfo> &seats, int step) {
  if (seats.isEmpty())
    return {};

  // Sort by suffix then number
  std::sort(seats.begin(), seats.end(),
            [](const CalcStock::SeatInfo &a, const CalcStock::SeatInfo &b) {
              if (a.suffix != b.suffix)
                return a.suffix < b.suffix;
              return a.num < b.num;
            });

  QList<QList<SeatInfo>> groups;
  QList<SeatInfo> currentGroup;
  currentGroup.append(seats[0]);

  for (int i = 1; i < seats.size(); ++i) {
    const SeatInfo &prev = seats[i - 1];
    const SeatInfo &curr = seats[i];

    bool isConsecutive =
        (curr.num == prev.num + step) && (curr.suffix == prev.suffix);
    if (isConsecutive) {
      currentGroup.append(curr);
    } else {
      groups.append(currentGroup);
      currentGroup.clear();
      currentGroup.append(curr);
    }
  }
  groups.append(currentGroup);
  return groups;
}

QString CalcStock::generateReportContent(const QString &basePath,
                                         bool oddEvenMode) {
  if (basePath.isEmpty())
    return "";

  QString ticketsPath = basePath + "/- Tickets -";
  if (!QDir(ticketsPath).exists()) {
    return "ERROR_NO_TICKETS_FOLDER";
  }

  int step = oddEvenMode ? 2 : 1;
  QStringList reportBody;
  int grandTotal = 0;

  struct ProcessItem {
    QString name;
    QStringList pdfs;
    QString path;
  };
  QList<ProcessItem> itemsToProcess;

  // Check for loose PDFs in - Tickets - root
  QDir ticketsDir(ticketsPath);
  QStringList loosePdfs =
      ticketsDir.entryList(QStringList() << "*.pdf", QDir::Files);
  if (!loosePdfs.isEmpty()) {
    itemsToProcess.append({"- Extra without folder -", loosePdfs, ticketsPath});
  }

  // Check subdirectories
  QStringList subDirs = ticketsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

  for (const QString &d : subDirs) {
    QString subPath = ticketsPath + "/" + d;
    QDir subDir(subPath);
    QStringList pdfs = subDir.entryList(QStringList() << "*.pdf", QDir::Files);
    if (!pdfs.isEmpty()) {
      itemsToProcess.append({d, pdfs, subPath});
    }
  }

  // Process logic
  for (const auto &item : itemsToProcess) {
    reportBody.append(QString("📂 %1").arg(item.name));
    reportBody.append(QString("🥅 TOTAL: %1").arg(item.pdfs.size()));
    grandTotal += item.pdfs.size();

    // Organize by Sector -> Row -> List of Seats
    QMap<QString, QMap<QString, QList<CalcStock::SeatInfo>>> data;
    QMap<QString, QString> priceMap; // Key: "Sec|Row"

    for (const QString &f : item.pdfs) {
      auto [sec, row, seatStr] = extractSrsFromFilename(f);
      if (!sec.isEmpty()) {
        auto [num, suffix] = parseSeatDetailed(seatStr);
        data[sec][row].append({QString::number(num) + suffix, num, suffix});

        QString key = sec + "|" + row;
        if (!priceMap.contains(key)) {
          priceMap[key] = extractFvFromFilename(f);
        }
      }
    }

    // Generate text for this category
    for (auto itSec = data.begin(); itSec != data.end(); ++itSec) {
      QString sec = itSec.key();
      QStringList secCounts;
      QStringList secGroupsInfo;
      int secTotal = 0;

      // Sort rows naturally
      QStringList rows = itSec.value().keys();
      std::sort(rows.begin(), rows.end(),
                [](const QString &a, const QString &b) {
                  // Try number sort if possible
                  bool ok1, ok2;
                  int n1 = a.toInt(&ok1);
                  int n2 = b.toInt(&ok2);
                  if (ok1 && ok2)
                    return n1 < n2;
                  return a < b;
                });

      for (const QString &row : rows) {
        QList<CalcStock::SeatInfo> &seats = itSec.value()[row];
        auto groups = findConsecutiveGroups(seats, step);

        for (const auto &g : groups) {
          int qty = g.size();
          secTotal += qty;
          secCounts.append(QString::number(qty));

          QString range;
          if (qty > 1)
            range = QString("%1/%2").arg(g.first().raw, g.last().raw);
          else
            range = g.first().raw;

          QString price = priceMap.value(sec + "|" + row, "N/A");
          secGroupsInfo.append(QString("💺Row: %1 Seat: %2 Qty: %3 [%4]")
                                   .arg(row, range)
                                   .arg(qty)
                                   .arg(price));
        }
      }

      QString breakdown = secCounts.join("+");
      reportBody.append(QString("🎫 Sector: %1 Total: %2 | %3")
                            .arg(sec)
                            .arg(secTotal)
                            .arg(breakdown));
      reportBody.append(secGroupsInfo);
    }
    reportBody.append(""); // Empty line
  }

  QStringList header;
  QString eventName = QFileInfo(basePath).fileName();
  header << QString("⚽ EVENT: %1").arg(eventName);
  header << QString("📅 DATE: %1")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
  header << QString("🥅 GRAND TOTAL: %1").arg(grandTotal);
  header << "========================================\n";

  return header.join("\n") + "\n" + reportBody.join("\n");
}

void CalcStock::runCalculation() {
  // Security Check
  SecurityManager::instance().checkAndAct();

  QString basePath = m_pathInput->text().trimmed();
  if (basePath.isEmpty()) {
    log("❌ ERROR: Please select a folder.", true);
    return;
  }

  log("⏳ Processing...", true);

  QString finalReport =
      generateReportContent(basePath, m_oddEvenMode->isChecked());

  if (finalReport == "ERROR_NO_TICKETS_FOLDER") {
    log("❌ ERROR: '- Tickets -' folder not found.", true);
    return;
  }

  // Setup output
  QString desktop =
      QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
  QString stockFolder = desktop + "/Stock Files";
  QDir().mkpath(stockFolder);

  QString eventName = QFileInfo(basePath).fileName();
  QString safeEventName = eventName;
  safeEventName.remove(QRegularExpression(R"([\\/*?:"<>|])"));
  QString outputFile =
      QString("%1/Stock_%2.txt").arg(stockFolder, safeEventName);

  QFile file(outputFile);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out << finalReport;
    file.close();
    log(finalReport);
    log(QString("\n✅ SUCCESS! Report saved to:\n%1").arg(outputFile));
  } else {
    log("❌ ERROR: Could not save report file.");
  }
}

} // namespace GOL
