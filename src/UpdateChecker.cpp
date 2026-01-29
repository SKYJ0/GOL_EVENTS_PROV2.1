#include "UpdateChecker.h"
#include "Utils.h"
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace GOL {

// Obfuscated:
// [ENCRYPTED URL STORED IN BINARY]
const QString UpdateChecker::UPDATE_JSON_URL = Utils::decryptString(
    QByteArray::fromHex("2913330A3E5638416C5E7D01290E3D0A34133F134406570E3F1D4A"
                        "655F250F27187673676D7311717870636D7E627315761B7D706974"
                        "58774A6D052213211F380975506D4574"));

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
      m_currentVersion(Utils::VERSION) {}

void UpdateChecker::checkForUpdates() {
  QNetworkRequest request(UPDATE_JSON_URL);
  request.setTransferTimeout(5000);

  QNetworkReply *reply = m_networkManager->get(request);
  QEventLoop loop;
  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (reply->error() != QNetworkReply::NoError) {
    emit checkFailed("Could not connect to update server");
    reply->deleteLater();
    return;
  }

  QByteArray responseData = reply->readAll();
  reply->deleteLater();

  QJsonDocument doc = QJsonDocument::fromJson(responseData);
  if (doc.isNull() || !doc.isObject()) {
    emit checkFailed("Invalid update data");
    return;
  }

  QJsonObject obj = doc.object();
  QString latestVersion = obj.value("version").toString();
  QString downloadUrl = obj.value("download_url").toString();

  // Semantic Version Comparison
  // Remove 'v' prefix if present
  QString local = m_currentVersion;
  if (local.startsWith("v"))
    local.remove(0, 1);

  QString remote = latestVersion;
  if (remote.startsWith("v"))
    remote.remove(0, 1);

  QStringList localParts = local.split('.');
  QStringList remoteParts = remote.split('.');

  bool updateNeeded = false;
  int length = std::max(localParts.size(), remoteParts.size());

  for (int i = 0; i < length; ++i) {
    int l = (i < localParts.size()) ? localParts[i].toInt() : 0;
    int r = (i < remoteParts.size()) ? remoteParts[i].toInt() : 0;

    if (r > l) {
      updateNeeded = true;
      break;
    } else if (r < l) {
      updateNeeded = false; // Remote is older
      break;
    }
  }

  if (updateNeeded) {
    emit updateAvailable(latestVersion, downloadUrl);
  } else {
    emit noUpdateAvailable();
  }
}

} // namespace GOL
