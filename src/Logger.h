#ifndef LOGGER_H
#define LOGGER_H

#include <QMutex>
#include <QObject>
#include <QString>


namespace GOL {

class Logger : public QObject {
  Q_OBJECT

public:
  static Logger &instance();

  // Log a message (writes to file and emits signal)
  void log(const QString &message);

signals:
  // Signal emitted when a new log message is added
  void logAdded(const QString &message);

private:
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  QMutex m_mutex;
};

} // namespace GOL

#endif // LOGGER_H
