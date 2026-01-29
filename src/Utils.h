#ifndef UTILS_H
#define UTILS_H

#include <QCoreApplication>
#include <QDir>
#include <QString>

class QLayout; // Forward declare globally

namespace GOL {

class Utils {
public:
  // Get resource path (handles both development and deployed scenarios)
  static QString resourcePath(const QString &relativePath);

  // Obfuscation
  static QString decryptString(const QByteArray &encrypted);

  // Get HWID (Hardware ID) for license validation
  static QString getHWID();

  // Get public IP address
  static QString getPublicIP();

  // Color constants matching Python design
  static const QString ACCENT_COLOR;
  static const QString ACCENT_GRADIENT;
  static const QString SIDEBAR_BG;
  static const QString PRO_COLOR;
  static const QString BG_COLOR;
  static const QString CARD_BG;
  static const QString CARD_BORDER;
  static const QString LOCKED_BG;
  static const QString SUCCESS_COLOR;
  static const QString ERROR_COLOR;
  static const QString NAV_HOVER;
  static const QString NAV_ACTIVE;

  // Animation Helper
  static void animateLayoutItems(QLayout *layout);

  // Version
  static void logToFile(const QString &message);
  static const QString VERSION;
};

} // namespace GOL

#endif // UTILS_H
