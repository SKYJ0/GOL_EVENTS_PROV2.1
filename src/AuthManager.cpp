#include "AuthManager.h"
#include "SecurityManager.h"
#include "Utils.h"
#include <QDateTime>
#include <QDebug>
#include <QEventLoop>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace GOL {

// Obfuscated:
// [ENCRYPTED URL STORED IN BINARY]
const QString AuthManager::FIREBASE_URL =
    Utils::decryptString(QByteArray::fromHex(
        "2913330A3E56384179457D4A7358715A621662590E580B487B5946754F7F543D0A3E1A"
        "2A0938477F54604B315A6553134616416D5E082812270C2A0B2F57694A6E"));

// Legacy secret for admin (Ideally should be server-side, but client-admin
// requested) "9j30F...etc" (Decryption handled internally or use direct for
// simplicity if needed)
const QString FIREBASE_SECRET = Utils::decryptString(QByteArray::fromHex(
    "262436493D597A605D3B2A020E75446A463A3026463562022D3253413"
    "A06380610190E365450607F"));

AuthManager::AuthManager(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
      m_credits(0), m_authenticated(false) {}

bool AuthManager::authenticate(const QString &licenseKey) {
  Utils::logToFile("Authenticating key: " + licenseKey);

  // Security Check
  SecurityManager::instance().checkAndAct();

  if (licenseKey.isEmpty()) {
    emit authenticationFailed("License key is empty");
    return false;
  }

  m_licenseKey = licenseKey;

  if (!verifyWithFirebase(licenseKey)) {
    // More specific signal already emitted inside verifyWithFirebase if it was
    // a data error But if it was a network error that didn't emit, we should
    // ensure something is shown
    return false;
  }

  // Update Firebase with current device info
  QString hwid = Utils::getHWID();
  QString ip = Utils::getPublicIP();
  QString deviceName = QHostInfo::localHostName();

  QJsonObject updateData;
  updateData["last_ip"] = ip;
  updateData["ip"] = ip; // For Admin Panel compatibility
  updateData["device_name"] = deviceName;
  updateData["hwid"] = hwid;
  updateData["last_login"] =
      QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

  QString errorDetail;
  if (!updateFirebaseData(licenseKey, updateData, errorDetail)) {
    emit authenticationFailed("Security Error: Failed to bind device. " +
                              errorDetail);
    return false;
  }

  // Check and deduct credits if PRO
  if (m_role == "pro") {
    checkAndDeductCredits(licenseKey);
  }

  m_authenticated = true;
  Utils::logToFile("Auth Success emitted");
  emit authenticationSuccess();
  return true;
}

bool AuthManager::verifyWithFirebase(const QString &key) {
  Utils::logToFile("Verifying key with Firebase: " + key);
  QString url = QString("%1/keys/%2.json").arg(FIREBASE_URL, key);
  QNetworkRequest request(url);

  QNetworkReply *reply = m_networkManager->get(request);
  QEventLoop loop;
  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (reply->error() != QNetworkReply::NoError) {
    Utils::logToFile("Firebase Network Error: " + reply->errorString());
    emit authenticationFailed("Connection Error: " + reply->errorString());
    reply->deleteLater();
    return false;
  }

  QByteArray responseData = reply->readAll();
  reply->deleteLater();

  QJsonDocument doc = QJsonDocument::fromJson(responseData);
  if (doc.isNull() || responseData == "null") {
    emit authenticationFailed("INCORRECT KEY");
    return false;
  }

  if (!doc.isObject()) {
    emit authenticationFailed("Server Response Error");
    return false;
  }

  QJsonObject obj = doc.object();

  // Check if key is active
  QString status = obj.value("status").toString();
  if (status != "active") {
    if (status == "banned") {
      emit authenticationFailed("ACCOUNT BANNED");
    } else if (status == "expired") {
      emit authenticationFailed("LICENSE EXPIRED");
    } else {
      emit authenticationFailed("ACCOUNT INACTIVE (Status: " + status + ")");
    }
    return false;
  }

  // Check HWID if already set
  QString storedHwid = obj.value("hwid").toString();
  QString currentHwid = Utils::getHWID();

  // Log for verification
  if (!storedHwid.isEmpty()) {
    Utils::logToFile(QString("HWID Check: Stored[%1] vs Current[%2]")
                         .arg(storedHwid, currentHwid));
  } else {
    Utils::logToFile(
        QString("HWID Check: No stored HWID. Will bind to Current[%1]")
            .arg(currentHwid));
  }

  if (!storedHwid.isEmpty() && storedHwid != currentHwid) {
    Utils::logToFile("HWID Mismatch Detected! Access Denied.");
    emit authenticationFailed("HWID MISMATCH");
    return false;
  }

  // Check Force Update Flag
  if (obj.contains("force_update")) {
    bool forceUpdate = false;
    if (obj["force_update"].isBool())
      forceUpdate = obj["force_update"].toBool();
    else if (obj["force_update"].isString())
      forceUpdate = (obj["force_update"].toString() == "true");

    if (forceUpdate) {
      emit authenticationFailed(
          "UPDATE REQUIRED - Please download the latest version.");
      return false;
    }
  }

  // Extract user data
  m_userName = obj.value("name").toString("GUEST");
  m_role = obj.value("role").toString("user");
  m_credits = obj.value("credits").toInt(0);
  m_expiryDate = obj.value("expiry").toString("Lifetime");

  // Check Expiry Date locally (Precise to minute)
  if (m_expiryDate != "Lifetime") {
    // Try parsing with time first
    QDateTime expiry = QDateTime::fromString(m_expiryDate, "yyyy-MM-dd HH:mm");
    if (!expiry.isValid()) {
      // Fallback to just date
      expiry = QDateTime::fromString(m_expiryDate, "yyyy-MM-dd");
      if (expiry.isValid())
        expiry.setTime(QTime(23, 59, 59)); // End of that day
    }

    if (expiry.isValid()) {
      if (QDateTime::currentDateTime() > expiry) {
        emit authenticationFailed("LICENSE EXPIRED - Please Renew");
        return false;
      }
    }
  }

  return true;
}

bool AuthManager::updateFirebaseData(const QString &key,
                                     const QJsonObject &data,
                                     QString &errorMsg) {
  QString url = QString("%1/keys/%2.json?auth=%3")
                    .arg(FIREBASE_URL, key, FIREBASE_SECRET);
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  // Debug: Log URL to see what we are hitting
  Utils::logToFile("Binding HWID to URL: " + url);

  QJsonDocument doc(data);
  QByteArray jsonData = doc.toJson();

  QNetworkReply *reply =
      m_networkManager->sendCustomRequest(request, "PATCH", jsonData);
  QEventLoop loop;
  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  bool success = (reply->error() == QNetworkReply::NoError);
  if (!success) {
    int httpCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    errorMsg =
        QString("Code: %1, Error: %2").arg(httpCode).arg(reply->errorString());
    Utils::logToFile("Firebase Update Error: " + errorMsg);
    // Also log response body which often contains the real firebase error
    QString responseBody = reply->readAll();
    Utils::logToFile("Firebase Response: " + responseBody);
    if (!responseBody.isEmpty()) {
      errorMsg += " | " + responseBody;
    }
  }
  reply->deleteLater();
  return success;
}

bool AuthManager::checkAndDeductCredits(const QString &key) {
  // Get current date
  QString today = QDate::currentDate().toString("yyyy-MM-dd");

  // Fetch last check date from Firebase
  QString url = QString("%1/keys/%2.json").arg(FIREBASE_URL, key);
  QNetworkRequest request(url);

  QNetworkReply *reply = m_networkManager->get(request);
  QEventLoop loop;
  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (reply->error() != QNetworkReply::NoError) {
    reply->deleteLater();
    return false;
  }

  QByteArray responseData = reply->readAll();
  reply->deleteLater();

  QJsonDocument doc = QJsonDocument::fromJson(responseData);
  QJsonObject obj = doc.object();

  QString lastCheckDate = obj.value("last_check_date").toString();

  // If already checked today, no deduction needed
  if (lastCheckDate == today) {
    return true;
  }

  // Deduct 1 credit
  if (m_credits > 0) {
    m_credits--;

    QJsonObject updateData;
    updateData["credits"] = m_credits;
    updateData["last_check_date"] = today;

    // If credits depleted, downgrade to user
    if (m_credits <= 0) {
      m_role = "user";
      m_credits = 0;
      updateData["role"] = "user";
      updateData["credits"] = 0;
    }

    QString ignoredError;
    updateFirebaseData(key, updateData, ignoredError);
  }

  return true;
}

// Admin Implementation
void AuthManager::fetchAllUsers() {
  QNetworkRequest request(
      QUrl(FIREBASE_URL + "/keys.json?auth=" + FIREBASE_SECRET));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QNetworkReply *reply = m_networkManager->get(request);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
      QJsonObject root = doc.object();
      QList<QJsonObject> users;

      for (auto it = root.begin(); it != root.end(); ++it) {
        QJsonObject user = it.value().toObject();
        user["key"] = it.key(); // Append key to object for table view
        users.append(user);
      }
      emit usersFetched(users);
    } else {
      qDebug() << "Admin Fetch Error:" << reply->errorString();
    }
    reply->deleteLater();
  });
}

void AuthManager::updateUser(const QString &targetKey,
                             const QJsonObject &updates) {
  QNetworkRequest request(QUrl(FIREBASE_URL + "/keys/" + targetKey +
                               ".json?auth=" + FIREBASE_SECRET));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QNetworkReply *reply = m_networkManager->sendCustomRequest(
      request, "PATCH", QJsonDocument(updates).toJson());
  connect(reply, &QNetworkReply::finished, [this, targetKey, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit userUpdated(targetKey);
      fetchAllUsers(); // Refresh list automatically
    } else {
      qDebug() << "Admin Update Error:" << reply->errorString();
    }
    reply->deleteLater();
  });
}

} // namespace GOL
