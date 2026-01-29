#include "RenamerFv.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>

namespace GOL {

RenamerFv::RenamerFv(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("GOLEVENTS - TICKET RENAMER PRO 🏷️");
  resize(900, 750);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(40, 30, 40, 40);
  mainLayout->setSpacing(20);

  // Header
  QLabel *title = new QLabel("🏷️ TICKET RENAMER + FV");
  title->setStyleSheet(QString("font-size: 30px; font-weight: bold; color: %1;")
                           .arg(Utils::ACCENT_COLOR));
  mainLayout->addWidget(title);

  QLabel *sub = new QLabel(
      "Auto-detect prices inside PDFs and rename files using strict format.");
  sub->setStyleSheet("font-size: 13px; color: gray;");
  mainLayout->addWidget(sub);

  // Folder selection
  QHBoxLayout *pathLayout = new QHBoxLayout();
  m_pathEdit = new QLineEdit();
  m_pathEdit->setPlaceholderText("Click Browse to select folder...");
  m_pathEdit->setReadOnly(true);
  m_pathEdit->setFixedHeight(45);
  m_pathEdit->setStyleSheet(
      QString("background-color: #0d0d0d; border: 1px solid %1; border-radius: "
              "10px; padding: 0 15px;")
          .arg(Utils::CARD_BORDER));

  m_btnBrowse = new QPushButton("BROWSE");
  m_btnBrowse->setFixedSize(120, 45);
  m_btnBrowse->setStyleSheet(QString("background-color: %1; color: white; "
                                     "border-radius: 10px; font-weight: bold;")
                                 .arg(Utils::ACCENT_COLOR));

  pathLayout->addWidget(m_pathEdit);
  pathLayout->addWidget(m_btnBrowse);
  mainLayout->addLayout(pathLayout);

  // Run button
  m_btnRun = new QPushButton("🚀 START BULK RENAMING");
  m_btnRun->setFixedHeight(60);
  m_btnRun->setStyleSheet(
      "background-color: #10b981; color: black; border-radius: 15px; "
      "font-weight: bold; font-size: 16px;");
  mainLayout->addWidget(m_btnRun);

  // Log area
  m_logArea = new QTextEdit();
  m_logArea->setReadOnly(true);
  m_logArea->setFont(QFont("Consolas", 12));
  m_logArea->setStyleSheet(
      QString("background-color: #050505; color: #00FF88; border: 1px solid "
              "%1; border-radius: 10px; padding: 10px;")
          .arg(Utils::CARD_BORDER));
  mainLayout->addWidget(m_logArea);

  // Connections
  connect(m_btnBrowse, &QPushButton::clicked, this, &RenamerFv::browseFolder);
  connect(m_btnRun, &QPushButton::clicked, this, &RenamerFv::startProcess);
}

void RenamerFv::log(const QString &msg) { m_logArea->append("> " + msg); }

void RenamerFv::browseFolder() {
  QString p = QFileDialog::getExistingDirectory(this, "Select folder");
  if (!p.isEmpty())
    m_pathEdit->setText(p);
}

QString RenamerFv::extractFaceValue(const QString &pdfPath) {
  try {
    poppler::document *doc =
        poppler::document::load_from_file(pdfPath.toStdString());
    if (!doc)
      return "";

    QString fullText;
    int numPages = doc->pages();
    for (int i = 0; i < numPages; ++i) {
      poppler::page *p = doc->create_page(i);
      if (p) {
        poppler::byte_array ba = p->text().to_utf8();
        fullText += QString::fromUtf8(ba.data(), ba.size()) + "\n";
        delete p;
      }
    }
    delete doc;

    if (fullText.isEmpty())
      return "";

    // Clean text: remove spaces between numbers (e.g., 1 5 0 . 0 0 -> 150.00)
    QRegularExpression spaceNumRe("(\\d)\\s*([.,])\\s*(\\d)");
    fullText.replace(spaceNumRe, "\\1\\2\\3");

    // Pattern for Prices: 1 to 4 digits before dot, exactly 2 digits after
    QRegularExpression pricePattern("\\b\\d{1,4}[.,]\\d{2}\\b");

    // 1. Target search with Keywords
    QString keywords =
        "(?:TOTALE|TOTAL|IMPORTO|VALORE|PREZZO|PRICE|PRIX|PRECIO)";
    QRegularExpression searchPattern(
        keywords +
            "\\s*(?:TICKET|AMOUNT|EUR|€)?[:\\s€]*(\\b\\d{1,4}[.,]\\d{2}\\b)",
        QRegularExpression::CaseInsensitiveOption);

    auto it = searchPattern.globalMatch(fullText);
    QString lastMatch;
    while (it.hasNext()) {
      lastMatch = it.next().captured(1);
    }

    if (!lastMatch.isEmpty()) {
      return lastMatch.replace(",", ".");
    }

    // 2. Fallback: Get the HIGHEST number matching the format
    auto itAll = pricePattern.globalMatch(fullText);
    double maxP = -1.0;
    while (itAll.hasNext()) {
      QString cand = itAll.next().captured(0).replace(",", ".");
      double val = cand.toDouble();
      if (val > maxP)
        maxP = val;
    }

    if (maxP >= 0) {
      return QString::number(maxP, 'f', 2);
    }

    return "";
  } catch (...) {
    return "";
  }
}

void RenamerFv::startProcess() {
  // Security Check
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
  log("Scanning folder: " + folder);

  QDir dir(folder);
  QStringList files = dir.entryList({"*.pdf"}, QDir::Files);

  int successCount = 0;
  for (const QString &filename : files) {
    if (filename.toUpper().contains("-FV"))
      continue; // Skip already processed

    QString fullPath = dir.absoluteFilePath(filename);
    QString price = extractFaceValue(fullPath);

    if (!price.isEmpty()) {
      QString cleanPrice = price;
      cleanPrice.replace(".", "p");

      QString baseName = QFileInfo(filename).baseName();
      QString newFilename = baseName + "-FV" + cleanPrice + ".pdf";

      if (dir.rename(filename, newFilename)) {
        log(QString("SUCCESS: %1 ⮕ FV %2€").arg(filename).arg(price));
        successCount++;
      } else {
        log(QString("ERROR: Could not rename %1").arg(filename));
      }
    } else {
      log(QString("SKIPPED: Format XX.YY not found in %1").arg(filename));
    }

    QApplication::processEvents(); // Keep UI responsive
  }

  log(QString("\n✅ FINISHED! Total %1 tickets renamed.").arg(successCount));
  m_btnRun->setEnabled(true);
  m_btnRun->setText("🚀 START BULK RENAMING");

  QMessageBox::information(
      this, "Success",
      QString("Renaming Complete!\nProcessed %1 files.").arg(successCount));
  QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

} // namespace GOL
