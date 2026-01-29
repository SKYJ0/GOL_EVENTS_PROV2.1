#ifndef SECURITYMANAGER_H
#define SECURITYMANAGER_H

#include <QObject>

namespace GOL {

class SecurityManager : public QObject {
  Q_OBJECT
public:
  static SecurityManager &instance();

  // Run all security checks. Returns true if secure, false if threat detected.
  bool performChecks();

  // Check and terminate if threat detected
  void checkAndAct();

signals:
  void threatDetected(const QString &type);

private:
  explicit SecurityManager(QObject *parent = nullptr);

  bool isDebuggerPresent();
  void terminateApp();
};

} // namespace GOL

#endif // SECURITYMANAGER_H
