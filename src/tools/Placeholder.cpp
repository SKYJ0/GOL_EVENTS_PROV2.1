#include "Placeholder.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QUrl>
#include <QVBoxLayout>

namespace GOL {

Placeholder::Placeholder(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("GOLEVENTS - PDF PLACEHOLDER PRO 🛠️");
  resize(850, 750);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(40, 30, 40, 40);
  mainLayout->setSpacing(20);

  // Header
  QLabel *titleLabel = new QLabel("🛠️ PDF PLACEHOLDER PRO");
  titleLabel->setStyleSheet(
      QString("font-size: 30px; font-weight: bold; color: %1;")
          .arg(Utils::ACCENT_COLOR));
  mainLayout->addWidget(titleLabel);

  QLabel *subTitle =
      new QLabel("Generate empty PDFs with custom names for your inventory.");
  subTitle->setStyleSheet("font-size: 13px; color: gray;");
  mainLayout->addWidget(subTitle);

  // Input area
  mainLayout->addWidget(new QLabel("Paste names list (one per line):"));
  m_inputArea = new QTextEdit();
  m_inputArea->setFont(QFont("Consolas", 13));
  m_inputArea->setStyleSheet(QString("background-color: %1; border: 1px solid "
                                     "%2; border-radius: 15px; padding: 10px;")
                                 .arg(Utils::CARD_BG, Utils::CARD_BORDER));
  mainLayout->addWidget(m_inputArea);

  // Buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  m_btnGenerate = new QPushButton("🚀 CHOOSE FOLDER & GENERATE");
  m_btnGenerate->setFixedHeight(55);
  m_btnGenerate->setCursor(Qt::PointingHandCursor);
  m_btnGenerate->setStyleSheet(
      "QPushButton { background-color: #10b981; color: black; border-radius: "
      "12px; font-weight: bold; font-size: 16px; }"
      "QPushButton:hover { background-color: #059669; }");

  m_btnClear = new QPushButton("🗑️ CLEAR");
  m_btnClear->setFixedSize(120, 55);
  m_btnClear->setCursor(Qt::PointingHandCursor);
  m_btnClear->setStyleSheet("QPushButton { background-color: #e74c3c; color: "
                            "white; border-radius: 12px; font-weight: bold; }"
                            "QPushButton:hover { background-color: #c0392b; }");

  btnLayout->addWidget(m_btnGenerate);
  btnLayout->addWidget(m_btnClear);
  mainLayout->addLayout(btnLayout);

  // Log area
  m_logArea = new QTextEdit();
  m_logArea->setReadOnly(true);
  m_logArea->setFixedHeight(120);
  m_logArea->setFont(QFont("Consolas", 12));
  m_logArea->setStyleSheet(
      QString("background-color: #050505; color: #00FF88; border: 1px solid "
              "%1; border-radius: 10px; padding: 10px;")
          .arg(Utils::CARD_BORDER));
  mainLayout->addWidget(m_logArea);

  // Connections
  connect(m_btnGenerate, &QPushButton::clicked, this,
          &Placeholder::processGeneration);
  connect(m_btnClear, &QPushButton::clicked, this, &Placeholder::clearAll);
}

void Placeholder::log(const QString &msg, bool clear) {
  if (clear)
    m_logArea->clear();
  m_logArea->append("> " + msg);
}

void Placeholder::clearAll() { m_inputArea->clear(); }

void Placeholder::processGeneration() {
  // Security Check
  SecurityManager::instance().checkAndAct();

  QString rawText = m_inputArea->toPlainText().trimmed();
  if (rawText.isEmpty()) {
    QMessageBox::warning(this, "Empty", "Please paste some names first!");
    return;
  }

  QString outputFolder =
      QFileDialog::getExistingDirectory(this, "Where to save these PDFs?");
  if (outputFolder.isEmpty()) {
    log("Operation cancelled by user.");
    return;
  }

  QStringList names = rawText.split('\n', Qt::SkipEmptyParts);
  log(QString("Starting process in: %1").arg(outputFolder), true);

  int successCount = 0;
  for (QString name : names) {
    name = name.trimmed();
    // Sanitize name: allow alnum, - and _
    QString safeName;
    for (QChar c : name) {
      if (c.isLetterOrNumber() || c == '-' || c == '_') {
        safeName += c;
      }
    }

    if (safeName.isEmpty()) {
      log(QString("Skipped invalid name: %1").arg(name));
      continue;
    }

    QString filePath = QDir(outputFolder).absoluteFilePath(safeName + ".pdf");

    // PDF Generation
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
      log(QString("FAILED: %1 | Error: Could not start painter").arg(name));
      continue;
    }

    // Setup fonts
    QFont titleFont("Helvetica", 30, QFont::Bold);
    QFont idFont("Helvetica", 12);

    QRect r = writer.pageLayout().paintRectPixels(writer.resolution());

    // Draw Watermark Text
    painter.setPen(QPen(QColor(200, 200, 200)));
    painter.setFont(titleFont);
    painter.drawText(r, Qt::AlignCenter, "PLACEHOLDER TICKET");

    // Draw Ticket ID
    painter.setPen(Qt::black);
    painter.setFont(idFont);
    painter.drawText(r.adjusted(0, 100, 0, 100), Qt::AlignCenter,
                     QString("ID: %1").arg(safeName));

    painter.end();

    successCount++;
    log(QString("SUCCESS: %1.pdf").arg(safeName));
  }

  log(QString("\n✅ COMPLETE! Created: %1/%2")
          .arg(successCount)
          .arg(names.size()));
  QMessageBox::information(
      this, "Success",
      QString("Process Finished!\n%1 PDFs created.").arg(successCount));

  QDesktopServices::openUrl(QUrl::fromLocalFile(outputFolder));
}

} // namespace GOL
