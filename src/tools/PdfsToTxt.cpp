#include "PdfsToTxt.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QDesktopServices>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace GOL {

PdfsToTxt::PdfsToTxt(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("GOLEVENTS - PDF NAME CLEANER PRO 📄");
  resize(800, 700);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(40, 30, 40, 40);
  mainLayout->setSpacing(20);

  // Header
  QLabel *titleLabel = new QLabel("📄 PDF NAME CLEANER");
  titleLabel->setStyleSheet(
      QString("font-size: 30px; font-weight: bold; color: %1;")
          .arg(Utils::ACCENT_COLOR));
  mainLayout->addWidget(titleLabel);

  QLabel *subTitle = new QLabel(
      "Extract 'Sector-Row-Seat' only from filenames into a TXT list.");
  subTitle->setStyleSheet("font-size: 13px; color: gray;");
  mainLayout->addWidget(subTitle);

  // Action Frame
  QHBoxLayout *actionLayout = new QHBoxLayout();
  m_pathEdit = new QLineEdit("No folder selected...");
  m_pathEdit->setReadOnly(true);
  m_pathEdit->setFixedHeight(45);
  m_pathEdit->setStyleSheet(
      QString("background-color: #0d0d0d; border: 1px solid %1; border-radius: "
              "8px; padding-left: 10px;")
          .arg(Utils::CARD_BORDER));

  m_btnBrowse = new QPushButton("BROWSE");
  m_btnBrowse->setFixedSize(120, 45);
  m_btnBrowse->setCursor(Qt::PointingHandCursor);
  m_btnBrowse->setStyleSheet(QString("background-color: %1; color: white; "
                                     "border-radius: 8px; font-weight: bold;")
                                 .arg(Utils::ACCENT_COLOR));

  actionLayout->addWidget(m_pathEdit);
  actionLayout->addWidget(m_btnBrowse);
  mainLayout->addLayout(actionLayout);

  // Process Button
  m_btnProcess = new QPushButton("🚀 START EXTRACTION & SAVE");
  m_btnProcess->setFixedHeight(55);
  m_btnProcess->setCursor(Qt::PointingHandCursor);
  m_btnProcess->setStyleSheet(
      "QPushButton { background-color: #10b981; color: black; border-radius: "
      "12px; font-weight: bold; font-size: 16px; }"
      "QPushButton:hover { background-color: #059669; }");
  mainLayout->addWidget(m_btnProcess);

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
  connect(m_btnBrowse, &QPushButton::clicked, this, &PdfsToTxt::browseFolder);
  connect(m_btnProcess, &QPushButton::clicked, this, &PdfsToTxt::runCleaner);
}

void PdfsToTxt::browseFolder() {
  QString dir = QFileDialog::getExistingDirectory(this, "Select PDF Folder");
  if (!dir.isEmpty()) {
    m_pathEdit->setText(dir);
  }
}

void PdfsToTxt::log(const QString &msg, bool clear) {
  if (clear)
    m_logArea->clear();
  m_logArea->append("> " + msg);
}

bool PdfsToTxt::naturalSort(const QString &a, const QString &b) {
  // Basic natural sort implementation for "Sector-Row-Seat"
  QStringList partsA = a.split('-');
  QStringList partsB = b.split('-');

  if (partsA.size() < 3 || partsB.size() < 3)
    return a < b;

  // Compare Sector (Alpha-numeric)
  // Logic: if both are numeric, compare as int. Else compare as string.
  // Python logic: if p1 is digit, p1=(0, int), else (1, p1).

  bool isNumA;
  int secNumA = partsA[0].toInt(&isNumA);

  bool isNumB;
  int secNumB = partsB[0].toInt(&isNumB);

  if (isNumA && isNumB) {
    if (secNumA != secNumB)
      return secNumA < secNumB;
  } else if (isNumA != isNumB) {
    // One is number, one is string. Number comes first (0 vs 1 prefix)
    return isNumA;
  } else {
    // Both strings
    if (partsA[0] != partsB[0])
      return partsA[0] < partsB[0];
  }

  // Compare Row (Numeric)
  int rowA = partsA[1].toInt();
  int rowB = partsB[1].toInt();
  if (rowA != rowB)
    return rowA < rowB;

  // Compare Seat (Numeric)
  return partsA[2].toInt() < partsB[2].toInt();
}

void PdfsToTxt::runCleaner() {
  // Security Check
  SecurityManager::instance().checkAndAct();

  QString folderPath = m_pathEdit->text();
  if (folderPath == "No folder selected..." || !QDir(folderPath).exists()) {
    QMessageBox::warning(this, "Warning",
                         "Please select a valid PDF folder first.");
    return;
  }

  log("Initializing process...", true);
  QStringList cleanNames;

  // Scan Engine
  QDirIterator it(folderPath, QStringList() << "*.pdf", QDir::Files,
                  QDirIterator::Subdirectories);
  QRegularExpression nameRegex("^([A-Za-z0-9]+-\\d+-\\d+)");

  while (it.hasNext()) {
    it.next();
    QString fileName = it.fileInfo().baseName();
    auto match = nameRegex.match(fileName);
    if (match.hasMatch()) {
      cleanNames.append(match.captured(1));
    }
  }

  if (cleanNames.isEmpty()) {
    log("❌ ERROR: No matching PDF names found (pattern: S-R-S).");
    return;
  }

  // Natural sort
  std::sort(cleanNames.begin(), cleanNames.end(), PdfsToTxt::naturalSort);

  // Save on Desktop
  QString desktopPath =
      QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
  QString outputFile = desktopPath + "/Clean_PDF_List.txt";

  QFile file(outputFile);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    for (const QString &name : cleanNames) {
      out << name << "\n";
    }
    file.close();

    log(QString("SUCCESS: %1 PDFs processed.").arg(cleanNames.size()));
    log(QString("Saved to Desktop: Clean_PDF_List.txt"));

    log("\nPreview (First 10):");
    for (int i = 0; i < std::min(10, (int)cleanNames.size()); ++i) {
      log(QString("   %1. %2").arg(i + 1, 2).arg(cleanNames[i]));
    }

    QMessageBox::information(
        this, "Done",
        QString("List generated successfully!\nTotal: %1\nCheck your Desktop.")
            .arg(cleanNames.size()));

    // Auto-open
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputFile));
  } else {
    log("❌ ERROR: Could not save file.");
  }
}

} // namespace GOL
