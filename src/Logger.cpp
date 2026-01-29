#include "Logger.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>


namespace GOL {

Logger &Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::log(const QString &message) {
  QMutexLocker locker(&m_mutex);

  QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
  QString fullMessage = QString("[%1] %2").arg(timestamp, message);

  // 1. Emit signal for UI
  emit logAdded(fullMessage);

  // 2. Write to file (same logic as old Utils::logToFile)
  QString desktopPath =
      QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
  QFile file(desktopPath + "/GOLEVENTS_DEBUG.log");
  if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    QTextStream stream(&file);
    // We might want a fuller timestamp for the file
    QString fileTimestamp =
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    stream << fileTimestamp << " " << message << "\n";
    file.close();
  }
}

} // namespace GOL
