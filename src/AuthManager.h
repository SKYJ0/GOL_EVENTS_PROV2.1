#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

namespace GOL {

class AuthManager : public QObject {
  Q_OBJECT

public:
  explicit AuthManager(QObject *parent = nullptr);

  // Authenticate with license key
  bool authenticate(const QString &licenseKey);

  // Getters
  QString getUserName() const { return m_userName; }
  QString getRole() const { return m_role; }
  int getCredits() const { return m_credits; }
  QString getExpiryDate() const { return m_expiryDate; }
  QString getLicenseKey() const { return m_licenseKey; }
  bool isAuthenticated() const { return m_authenticated; }

  // Admin Functions
  void fetchAllUsers();
  void updateUser(const QString &targetKey, const QJsonObject &updates);

  // Firebase URL
  static const QString FIREBASE_URL;

signals:
  void authenticationSuccess();
  void authenticationFailed(const QString &reason);
  void usersFetched(const QList<QJsonObject> &users);
  void userUpdated(const QString &key);

private:
  bool verifyWithFirebase(const QString &key);
  bool updateFirebaseData(const QString &key, const QJsonObject &data,
                          QString &errorMsg);
  bool checkAndDeductCredits(const QString &key);

  QNetworkAccessManager *m_networkManager;
  QString m_licenseKey;
  QString m_userName;
  QString m_role;
  int m_credits;
  QString m_expiryDate;
  bool m_authenticated;
};

} // namespace GOL

#endif // AUTHMANAGER_H
