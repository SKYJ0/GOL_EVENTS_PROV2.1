#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include "AuthManager.h"
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>


namespace GOL {

class AdminPanel : public QWidget {
  Q_OBJECT

public:
  explicit AdminPanel(AuthManager *authManager, QWidget *parent = nullptr);
  void refreshData();

private slots:
  void onUsersFetched(const QList<QJsonObject> &users);
  void onSearchChanged(const QString &text);
  void showContextMenu(const QPoint &pos);

  // Actions
  void banUser();
  void unbanUser();
  void resetHWID();
  void setRole();
  void renameUser();
  void copyKey();
  void forceUpdateUser();

private:
  void setupUI();
  void filterTable();

  AuthManager *m_authManager;
  QTableWidget *m_table;
  QLineEdit *m_searchInput;
  QPushButton *m_refreshBtn;

  QList<QJsonObject> m_allUsers; // Cache for filtering
  QString m_selectedKey;         // Currently selected user key
};

} // namespace GOL

#endif // ADMINPANEL_H
