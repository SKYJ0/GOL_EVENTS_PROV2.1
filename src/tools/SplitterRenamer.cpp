#include "SplitterRenamer.h"
#include "SecurityManager.h"
#include "Utils.h"
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QProcess>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-page.h>

// Debug macro
#include <QDebug>

namespace GOL {

SplitterRenamer::SplitterRenamer(QWidget *parent) : QDialog(parent) {
  setWindowTitle("GOL Events Pro - Splitter & Renamer");
  resize(800, 600);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(20);
  mainLayout->setContentsMargins(30, 30, 30, 30);

  // Title
  QLabel *title = new QLabel("PDF Splitter & Renamer");
  title->setStyleSheet("font-size: 24px; font-weight: bold; color: #FFFFFF;");
  title->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(title);

  // Instructions
  QLabel *info = new QLabel(
      "Select a folder containing tickets.\nThe tool will split them into "
      "single pages and rename them based on content (Sector-Row-Seat).");
  info->setStyleSheet("color: #AAAAAA; font-size: 14px;");
  info->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(info);

  // Path Selection
  QHBoxLayout *pathLayout = new QHBoxLayout();
  m_pathEdit = new QLineEdit();
  m_pathEdit->setPlaceholderText("Select folder...");
  m_pathEdit->setReadOnly(true);
  m_pathEdit->setFixedHeight(40);
  m_pathEdit->setStyleSheet(
      "background-color: #333; color: white; border-radius: 5px; padding: "
      "5px;");

  m_btnBrowse = new QPushButton("Browse");
  m_btnBrowse->setFixedHeight(40);
  m_btnBrowse->setStyleSheet(
      "background-color: #555; color: white; border-radius: 5px;");

  pathLayout->addWidget(m_pathEdit);
  pathLayout->addWidget(m_btnBrowse);
  mainLayout->addLayout(pathLayout);

  // Run button
  m_btnRun = new QPushButton("🚀 START BULK RENAMING");
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
          &SplitterRenamer::browseFolder);
  connect(m_btnRun, &QPushButton::clicked, this,
          &SplitterRenamer::startProcess);
}

void SplitterRenamer::log(const QString &msg) { m_logArea->append("> " + msg); }

void SplitterRenamer::browseFolder() {
  QString p = QFileDialog::getExistingDirectory(this, "Select folder");
  if (!p.isEmpty())
    m_pathEdit->setText(p);
}

bool SplitterRenamer::isAcMilanTicket(const QString &filename) {
  // Broad check for the structure we expect for validation
  QRegularExpression re("_(\\d+)_(\\d+)_(\\d+)_",
                        QRegularExpression::CaseInsensitiveOption);
  return re.match(filename).hasMatch();
}

SplitterRenamer::TicketInfo
SplitterRenamer::extractInfoFromFilename(const QString &filename) {
  TicketInfo info;
  // Expected format: ..._Gate_Sector_Row_Seat_...
  // Example: ACMILAN-USLECCE_10_157_2_1_FALLACI_000705005897741.pdf
  // Matches: 10 (Gate), 157 (Sector), 2 (Row), 1 (Seat)

  // Try matching 4 numbers first (Gate + Sector + Row + Seat)
  QRegularExpression re4("_(\\d+)_(\\d+)_(\\d+)_(\\d+)_");
  QRegularExpressionMatch match4 = re4.match(filename);

  if (match4.hasMatch()) {
    // pattern: _Gate_Sector_Row_Seat_
    // captured(1) = Gate
    // captured(2) = Sector
    // captured(3) = Row
    // captured(4) = Seat
    info.s = match4.captured(2);
    info.r = match4.captured(3);
    info.st = match4.captured(4);
    info.valid = true;
    return info;
  }

  // Fallback: Try 3 numbers (Sector + Row + Seat) if the 4-number pattern fails
  QRegularExpression re3("_(\\d+)_(\\d+)_(\\d+)_");
  QRegularExpressionMatch match3 = re3.match(filename);

  if (match3.hasMatch()) {
    info.s = match3.captured(1);
    info.r = match3.captured(2);
    info.st = match3.captured(3);
    info.valid = true;
  }
  return info;
}

SplitterRenamer::TicketInfo
SplitterRenamer::extractTicketInfo(const QString &pdfPath, int pageNum,
                                   const TicketInfo *hint) {
  TicketInfo info;
  try {
    poppler::document *doc =
        poppler::document::load_from_file(pdfPath.toStdString());
    if (!doc || pageNum >= doc->pages())
      return info;

    poppler::page *p = doc->create_page(pageNum);
    if (p) {
      // 1. Get text content
      poppler::byte_array ba = p->text().to_utf8();
      QString text = QString::fromUtf8(ba.data(), ba.size());
      delete p;

      QString row, seat, sector;

      // 0. PRIORITY CHECK: If we have a hint from filename (Sector), check if
      // it exists with Sector keywords
      if (hint && !hint->s.isEmpty()) {
        log(QString("   [?] Hint from filename - Sector: %1").arg(hint->s));

        // Look for "Settore ... 157" allowing for newlines, "BLOCK", etc.
        // Match "Settore", then up to 100 non-digit chars (skips "/ BLOCK:
        // \n"), then the number.
        QRegularExpression hintRe(
            QString(R"((?:SETTORE|BLOCK|SEC|SECTOR)(?:[^0-9]{0,100})\b%1\b)")
                .arg(QRegularExpression::escape(hint->s)),
            QRegularExpression::CaseInsensitiveOption);

        if (hintRe.match(text).hasMatch()) {
          sector = hint->s;
          log(QString("   [!] Priority Match Found: 'Settore ... %1'")
                  .arg(sector));
        } else {
          log("   [?] Hint not found in PDF text near 'Settore' keyword.");
        }
      }

      // 1. Try Regex Patterns (Most robust for labeled data)
      // Matches: "Fila 10", "Row: 10", "Fila. 10", "Fila/Row 10"
      QRegularExpression rowRe(R"((?:FILA|ROW)[^0-9\n]*(\d+))",
                               QRegularExpression::CaseInsensitiveOption);
      auto rowMatch = rowRe.match(text);
      if (rowMatch.hasMatch()) {
        row = rowMatch.captured(1);
      }

      // Matches: "Posto 10", "Seat: 10"
      QRegularExpression seatRe(R"((?:POSTO|SEAT)[^0-9\n]*(\d+))",
                                QRegularExpression::CaseInsensitiveOption);
      auto seatMatch = seatRe.match(text);
      if (seatMatch.hasMatch()) {
        seat = seatMatch.captured(1);
      }

      // Strategy 1.5: Aggressive Hint Match
      // If we haven't found the sector yet via "Settore ...", but we have a
      // filename hint (e.g. "259") Check if "259" exists ANYWHERE in the text
      // as a standalone word.
      if (sector.isEmpty() && hint && !hint->s.isEmpty()) {
        // Ensure it matches as a whole word
        QRegularExpression bareRe(
            QString(R"(\b%1\b)").arg(QRegularExpression::escape(hint->s)));
        if (bareRe.match(text).hasMatch()) {
          // To be safe, ensure it's not exactly the Row or Seat we just found
          // (if they are same number)
          if (hint->s != row && hint->s != seat) {
            sector = hint->s;
            log(QString("   [!] Found Hint number in text (Aggressive): %1")
                    .arg(sector));
          }
        }
      }

      // Matches: "Settore 123", "Block 123", "Sec 123"
      if (sector.isEmpty()) {
        QRegularExpression secRe(
            R"((?:SETTORE|BLOCK|SEC|SECTOR)[^0-9\n]*(\d+))",
            QRegularExpression::CaseInsensitiveOption);
        auto secMatch = secRe.match(text);
        if (secMatch.hasMatch()) {
          sector = secMatch.captured(1);
        }
      }

      // 2. Fallback: Context-Aware Line Scanning (Original Python logic
      // adaptation) Only runs if Regex failed to find something
      QStringList lines = text.split('\n');
      for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed().toUpper();

        // Row Fallback (Lookahead for standalone number)
        if (row.isEmpty() && (line.contains("FILA") || line.contains("ROW"))) {
          for (int j = 1; j <= 3 && (i + j) < lines.size(); ++j) {
            QString val = lines[i + j].trimmed();
            val.replace(":", "").replace(".", "");
            bool ok;
            int rVal = val.toInt(&ok);
            if (ok && rVal >= 0) {
              row = QString::number(rVal);
              break;
            }
          }
        }

        // Seat Fallback
        if (seat.isEmpty() &&
            (line.contains("POSTO") || line.contains("SEAT"))) {
          for (int j = 1; j <= 3 && (i + j) < lines.size(); ++j) {
            QString val = lines[i + j].trimmed();
            val.replace(":", "").replace(".", "");
            bool ok;
            int sVal = val.toInt(&ok);
            if (ok && sVal >= 0) {
              seat = QString::number(sVal);
              break;
            }
          }
        }

        // TABLE STRATEGY: Check for lines like "SETTORE FILA POSTO" then next
        // line "236 5 21" If line contains BOTH "Row" and "Seat" (or
        // Fila/Posto)
        if ((line.contains("FILA") || line.contains("ROW")) &&
            (line.contains("POSTO") || line.contains("SEAT"))) {
          // Look ahead 1-2 lines for a line with multiple numbers
          for (int j = 1; j <= 2 && (i + j) < lines.size(); ++j) {
            QString potentialValLine = lines[i + j].trimmed();
            // Split by space
            QStringList parts = potentialValLine.split(
                QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            // Filter for numeric parts
            QList<QString> numericParts;
            for (const QString &p : parts) {
              QString clean = p;
              bool ok;
              clean.toInt(&ok);
              if (ok)
                numericParts.append(clean);
            }

            // Expecting 3 numbers: Sector, Row, Seat (standard order)
            // Check for Gate in the header line itself
            bool hasGate = (line.contains("INGRESSO") || line.contains("GATE"));

            // Strategy 1: Gate + Sector + Row + Seat (Expect >= 4 numbers)
            if (hasGate && numericParts.size() >= 4) {
              // 12 328 6 6 -> Gate(0), Sector(1), Row(2), Seat(3)
              if (sector.isEmpty())
                sector = numericParts[1];
              if (row.isEmpty())
                row = numericParts[2];
              if (seat.isEmpty())
                seat = numericParts[3];
              break;
            }
            // Strategy 2: Sector + Row + Seat (Expect >= 3 numbers)
            else if (numericParts.size() >= 3) {
              // Assume standard order: Sector, Row, Seat
              // Or check headers for position? Usually it's Sector Row Seat
              if (sector.isEmpty())
                sector = numericParts[0];
              if (row.isEmpty())
                row = numericParts[1];
              if (seat.isEmpty())
                seat = numericParts[2];
              break;
            }
            // Strategy 3: Row + Seat (Fallback, Expect 2 numbers)
            else if (numericParts.size() == 2) {
              if (row.isEmpty())
                row = numericParts[0];
              if (seat.isEmpty())
                seat = numericParts[1];
              break;
            }
          }
        }
      }

      // 3. Sector Fallback: Standalone 3-digit number heuristic
      if (sector.isEmpty()) {
        QStringList candidateSectors;
        for (const QString &line : lines) {
          QString clean = line.trimmed();
          bool ok;
          clean.toInt(&ok); // Check if purely numeric
          if (ok && clean.length() == 3) {
            // Avoid if it matches row or seat exactly
            if (clean == row || clean == seat)
              continue;
            candidateSectors.append(clean);
          }
        }
        if (!candidateSectors.isEmpty())
          sector = candidateSectors.first();
      }

      // Final Check
      if (!row.isEmpty() && !seat.isEmpty() && !sector.isEmpty()) {
        info.s = sector;
        info.r = row;
        info.st = seat;
        info.valid = true;
      } else {
        info.s = sector.isEmpty() ? "?" : sector;
        info.r = row.isEmpty() ? "?" : row;
        info.st = seat.isEmpty() ? "?" : seat;
        info.valid = false;
      }
    }
    delete doc;
  } catch (...) {
  }
  return info;
}

void SplitterRenamer::startProcess() {
  SecurityManager::instance().checkAndAct();

  QString folder = m_pathEdit->text();
  if (folder.isEmpty() || !QDir(folder).exists()) {
    QMessageBox::warning(this, "Warning",
                         "Please select a valid folder first.");
    return;
  }

  m_btnRun->setEnabled(false);
  m_btnRun->setText("PROCESSING...");
  m_logArea->clear();

  QDir dir(folder);
  QString outputDir = dir.absoluteFilePath("Renamed_Result");
  if (!QDir(outputDir).exists())
    QDir().mkpath(outputDir);

  log("Scanning folder: " + folder);
  QStringList files = dir.entryList({"*.pdf"}, QDir::Files);

  int successCount = 0;
  int failCount = 0;

  for (const QString &filename : files) {
    QString fullPath = dir.absoluteFilePath(filename);

    // Extract ticket info ONLY from filename (no PDF opening at all)
    TicketInfo filenameInfo = extractInfoFromFilename(filename);

    QString newName;
    if (filenameInfo.valid) {
      // Create new name: Sector-Row-Seat
      newName = QString("%1-%2-%3.pdf")
                    .arg(filenameInfo.s)
                    .arg(filenameInfo.r)
                    .arg(filenameInfo.st);
      log(QString("Processing '%1' -> '%2'").arg(filename).arg(newName));
    } else {
      // Filename doesn't match expected pattern - skip or keep original
      log(QString("   [⚠️] Skipping '%1': Filename pattern not recognized.")
              .arg(filename));
      failCount++;
      continue;
    }

    // Handle duplicates -> append _1, _2
    QString finalName = newName;
    int counter = 1;
    while (QFile::exists(QDir(outputDir).absoluteFilePath(finalName))) {
      QString baseName = QString("%1-%2-%3")
                             .arg(filenameInfo.s)
                             .arg(filenameInfo.r)
                             .arg(filenameInfo.st);
      finalName = QString("%1_%2.pdf").arg(baseName).arg(counter++);
    }

    QString destPath = QDir(outputDir).absoluteFilePath(finalName);

    // Simple file copy - NO PDF OPENING OR MODIFICATION
    if (QFile::copy(fullPath, destPath)) {
      log(QString("   [+] Renamed: %1").arg(finalName));
      successCount++;
    } else {
      log(QString("   [!] Failed to copy: %1").arg(filename));
      failCount++;
    }

    QApplication::processEvents();
  }

  log(QString("\n✅ FINISHED! Success: %1, Failed: %2")
          .arg(successCount)
          .arg(failCount));
  m_btnRun->setEnabled(true);
  m_btnRun->setText("🚀 START BULK RENAMING");

  QMessageBox::information(
      this, "Done", "Process complete. Results in 'Renamed_Result' folder.");
  QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir));
}

} // namespace GOL
