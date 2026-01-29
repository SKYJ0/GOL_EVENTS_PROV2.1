#include "QrGenerator.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <qrencode.h>

namespace GOL {

QrGenerator::QrGenerator(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("QR Code Generator - GOLEVENTS");
  resize(700, 600);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QLabel *title = new QLabel("📱 QR CODE GENERATOR");
  title->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: bold;")
                           .arg(Utils::ACCENT_COLOR));
  mainLayout->addWidget(title);
  mainLayout->addSpacing(20);

  QLabel *instrLabel = new QLabel("Enter codes/URLs (one per line):");
  instrLabel->setStyleSheet("font-size: 14px;");
  mainLayout->addWidget(instrLabel);

  m_inputText = new QTextEdit();
  m_inputText->setPlaceholderText("ABC123\nDEF456\nGHI789");
  m_inputText->setStyleSheet("QTextEdit { background-color: #1a1a1a; color: "
                             "white; border: 1px solid #333; "
                             "border-radius: 8px; padding: 10px; font-family: "
                             "Consolas; font-size: 13px; }");
  mainLayout->addWidget(m_inputText);

  QHBoxLayout *btnLayout = new QHBoxLayout();

  QPushButton *folderBtn = new QPushButton("📁 Choose Output Folder");
  folderBtn->setStyleSheet(QString(
      "QPushButton { background-color: #333; color: white; border: none; "
      "padding: 12px 20px; border-radius: 8px; font-size: 14px; font-weight: "
      "bold; }"
      "QPushButton:hover { background-color: #444; }"));
  connect(folderBtn, &QPushButton::clicked, this, &QrGenerator::selectFolder);
  btnLayout->addWidget(folderBtn);

  QPushButton *generateBtn = new QPushButton("🚀 GENERATE QR CODES");
  generateBtn->setStyleSheet(
      QString("QPushButton { background-color: %1; color: white; border: none; "
              "padding: 12px 30px; border-radius: 8px; font-size: 14px; "
              "font-weight: bold; }"
              "QPushButton:hover { background-color: #2a7ab8; }")
          .arg(Utils::ACCENT_COLOR));
  connect(generateBtn, &QPushButton::clicked, this,
          &QrGenerator::generateQRCodes);
  btnLayout->addWidget(generateBtn);

  mainLayout->addLayout(btnLayout);

  m_statusLabel = new QLabel();
  m_statusLabel->setStyleSheet(
      QString("color: %1; font-size: 13px;").arg(Utils::SUCCESS_COLOR));
  mainLayout->addWidget(m_statusLabel);
}

void QrGenerator::selectFolder() {
  QString folder =
      QFileDialog::getExistingDirectory(this, "Select Output Folder");
  if (!folder.isEmpty()) {
    m_outputFolder = folder;
    m_statusLabel->setText(QString("Output folder: %1").arg(folder));
    m_statusLabel->setStyleSheet("color: " + Utils::ACCENT_COLOR +
                                 "; font-size: 13px;");
  }
}

void QrGenerator::generateQRCodes() {
  // Security Check
  SecurityManager::instance().checkAndAct();

  if (m_outputFolder.isEmpty()) {
    QMessageBox::warning(this, "Error",
                         "Please select an output folder first.");
    return;
  }

  QString text = m_inputText->toPlainText().trimmed();
  if (text.isEmpty()) {
    QMessageBox::warning(this, "Error", "Please enter at least one code.");
    return;
  }

  QStringList codes = text.split('\n', Qt::SkipEmptyParts);
  int count = 0;

  for (const QString &code : codes) {
    QString trimmedCode = code.trimmed();
    if (trimmedCode.isEmpty())
      continue;

    // Sanitize filename
    QString safeName = trimmedCode;
    safeName.replace(QRegularExpression("[<>:\"/\\\\|?*]"), "_");
    safeName = safeName.trimmed();
    if (safeName.length() > 100)
      safeName.truncate(100);
    if (safeName.isEmpty())
      safeName = "qr_code";

    // Generate QR code using libqrencode
    QRcode *qr = QRcode_encodeString(trimmedCode.toUtf8().constData(), 0,
                                     QR_ECLEVEL_H, QR_MODE_8, 1);

    if (qr) {
      // Create 1035x1035 image
      int size = 1035;
      int qrSize = qr->width;
      int scale = size / qrSize;

      QImage image(size, size, QImage::Format_RGB32);
      image.fill(Qt::white);

      QPainter painter(&image);
      painter.setPen(Qt::NoPen);
      painter.setBrush(Qt::black);

      for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
          if (qr->data[y * qrSize + x] & 1) {
            painter.drawRect(x * scale, y * scale, scale, scale);
          }
        }
      }

      // Save image
      QString filename = QString("%1/%2.png").arg(m_outputFolder, safeName);
      image.save(filename);

      QRcode_free(qr);
      count++;
    }
  }

  m_statusLabel->setText(
      QString("✅ Generated %1 QR codes successfully!").arg(count));
  m_statusLabel->setStyleSheet(
      QString("color: %1; font-size: 13px; font-weight: bold;")
          .arg(Utils::SUCCESS_COLOR));

  QMessageBox::information(this, "Success",
                           QString("Generated %1 QR codes!").arg(count));
}

} // namespace GOL
