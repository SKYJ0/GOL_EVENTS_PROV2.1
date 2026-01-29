#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "Utils.h"
#include "Logger.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QEventLoop>
#include <QFile>
// #include <QHostInfo>
#include <QGraphicsOpacityEffect>
#include <QLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QWidget>


namespace GOL {

// Color constants
// Color constants (Dribbble Inspired)
// Color constants (Sapphire & Steel Restored)
const QString Utils::ACCENT_COLOR = "#3b82f6";    // Blue 500
const QString Utils::ACCENT_GRADIENT = "#3b82f6"; // Solid Blue (No Gradient)
const QString Utils::SIDEBAR_BG = "#0f172a";      // Slate 900
const QString Utils::PRO_COLOR = "#fbbf24";       // Amber
const QString Utils::BG_COLOR = "#0f172a";        // Slate 900
const QString Utils::CARD_BG = "#1e293b";         // Slate 800
const QString Utils::CARD_BORDER = "#334155";
const QString Utils::LOCKED_BG = "#0f172a";
const QString Utils::SUCCESS_COLOR = "#10b981"; // Emerald
const QString Utils::ERROR_COLOR = "#ef4444";   // Red
const QString Utils::NAV_HOVER = "#1e293b";
const QString Utils::NAV_ACTIVE = "#1e293b";
const QString Utils::VERSION = "v2.3.3";

QString Utils::decryptString(const QByteArray &encrypted) {
  // Hidden key: "ANTIGRAV_STRONG_KEY_2026_V2"
  // Split into parts to avoid simple string searching
  const unsigned char key[] = {0x41, 0x4E, 0x54, 0x49, 0x47, 0x52, 0x41,
                               0x56, 0x5F, 0x53, 0x54, 0x52, 0x4F, 0x4E,
                               0x47, 0x5F, 0x4B, 0x45, 0x59, 0x5F, 0x32,
                               0x30, 0x32, 0x36, 0x5F, 0x56, 0x32};
  int keyLen = sizeof(key);

  QByteArray decrypted;
  decrypted.resize(encrypted.size());

  // We need to reverse the process
  // The Python encryption was:
  // encrypted[i] = input[i] ^ key[i % keyLen]
  // if i > 0: encrypted[i] ^= encrypted[i-1]

  // So for decryption:
  // We must process normally because the "encrypted[i-1]" dependency is on the
  // CIPHERTEXT which we have. wait, let's trace: Enc: C[i] =  (P[i] ^ K[i]) ^
  // C[i-1]  (for i > 0) Enc: C[0] = P[0] ^ K[0]

  // Dec:
  // We know C[i], K[i], and C[i-1]
  // P[i] ^ K[i] = C[i] ^ C[i-1]
  // P[i] = C[i] ^ C[i-1] ^ K[i]

  // For i=0:
  // P[0] = C[0] ^ K[0]

  for (int i = 0; i < encrypted.size(); ++i) {
    unsigned char encByte = (unsigned char)encrypted[i];
    unsigned char k = key[i % keyLen];
    unsigned char prevEnc = (i > 0) ? (unsigned char)encrypted[i - 1] : 0;

    unsigned char plainByte = encByte ^ k ^ prevEnc;
    decrypted[i] = plainByte;
  }

  return QString::fromUtf8(decrypted);
}

QString Utils::resourcePath(const QString &relativePath) {
  // Try application directory first
  QDir appDir(QCoreApplication::applicationDirPath());
  QString fullPath = appDir.filePath(relativePath);

  if (QFile::exists(fullPath)) {
    return fullPath;
  }

  // Try resource system
  QString resourcePath = ":/" + relativePath;
  if (QFile::exists(resourcePath)) {
    return resourcePath;
  }

  return fullPath; // Return anyway, caller will handle missing file
}

QString Utils::getHWID() {
  QProcess process;
  process.start("wmic", QStringList()
                            << "baseboard" << "get" << "serialnumber");

  if (process.waitForFinished(3000)) {
    QByteArray data = process.readAllStandardOutput();
    QString output = QString::fromLocal8Bit(data);
    QStringList lines =
        output.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

    if (lines.size() > 1) {
      QString hwid = lines[1].trimmed();
      if (!hwid.isEmpty()) {
        return hwid;
      }
    }
  }

  // Fallback: use hostname hash
  QString hostname = qEnvironmentVariable("COMPUTERNAME");
  if (hostname.isEmpty()) {
    hostname = "UNKNOWN_HOST";
  }
  QByteArray hash =
      QCryptographicHash::hash(hostname.toUtf8(), QCryptographicHash::Sha256);
  return hash.toHex().left(16);
}

QString Utils::getPublicIP() {
  Utils::logToFile("Fetching public IP...");
  QStringList urls = {"http://api.ipify.org", "https://api.ipify.org",
                      "https://ifconfig.me/ip"};

  for (const QString &url : urls) {
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setTransferTimeout(1500);

    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
      QString ip = reply->readAll().trimmed();
      reply->deleteLater();
      return ip;
    }

    reply->deleteLater();
  }

  Utils::logToFile("Public IP fetch failed, returning Unknown");
  return "Unknown";
}

#include "Logger.h"

// ... (keep this include at top of file, but tool replaces contextually)

void Utils::logToFile(const QString &message) {
  Logger::instance().log(message);
}

#include <QGraphicsOpacityEffect>
#include <QLayout>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

void Utils::animateLayoutItems(QLayout *layout) {
  if (!layout)
    return;

  for (int i = 0; i < layout->count(); ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if (QWidget *widget = item->widget()) {
      // Reset state
      QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(widget);
      eff->setOpacity(0);
      widget->setGraphicsEffect(eff);

      // Animate
      QPropertyAnimation *anim = new QPropertyAnimation(eff, "opacity");
      anim->setDuration(400);
      anim->setStartValue(0.0);
      anim->setEndValue(1.0);
      anim->setEasingCurve(QEasingCurve::OutQuad);

      // Stagger
      QTimer::singleShot(i * 50, [anim]() {
        anim->start(QAbstractAnimation::DeleteWhenStopped);
      });
    }
  }
}

} // namespace GOL
