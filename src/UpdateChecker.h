#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QString>
#include <QObject>
#include <QNetworkAccessManager>

namespace GOL {

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    
    // Check for updates from GitHub
    void checkForUpdates();
    
    // GitHub update URL
    static const QString UPDATE_JSON_URL;
    
signals:
    void updateAvailable(const QString& newVersion, const QString& downloadUrl);
    void noUpdateAvailable();
    void checkFailed(const QString& reason);
    
private:
    QNetworkAccessManager* m_networkManager;
    QString m_currentVersion;
};

} // namespace GOL

#endif // UPDATECHECKER_H
