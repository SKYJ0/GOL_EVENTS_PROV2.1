#include "AdminPanel.h"
#include "Utils.h"
#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QDateTime>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidgetItem>

namespace GOL {

AdminPanel::AdminPanel(AuthManager *authManager, QWidget *parent)
    : QWidget(parent), m_authManager(authManager) {
  setupUI();
  connect(m_authManager, &AuthManager::usersFetched, this,
          &AdminPanel::onUsersFetched);
}

void AdminPanel::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(15);

  // Header
  QHBoxLayout *headerLayout = new QHBoxLayout();
  QLabel *title = new QLabel("ADMIN DASHBOARD");
  title->setStyleSheet(
      "font-size: 24px; font-weight: bold; color: white; letter-spacing: 1px;");

  m_refreshBtn = new QPushButton("🔄 REFRESH");
  m_refreshBtn->setFixedWidth(120);
  m_refreshBtn->setStyleSheet(
      QString("QPushButton { background: %1; color: white; border-radius: 6px; "
              "font-weight: bold; padding: 8px; }"
              "QPushButton:hover { opacity: 0.9; }")
          .arg(Utils::ACCENT_GRADIENT));
  connect(m_refreshBtn, &QPushButton::clicked, this, &AdminPanel::refreshData);

  headerLayout->addWidget(title);
  headerLayout->addStretch();
  headerLayout->addWidget(m_refreshBtn);
  mainLayout->addLayout(headerLayout);

  // Filter
  m_searchInput = new QLineEdit();
  m_searchInput->setPlaceholderText("🔍 Search by Name, Key, Role...");
  m_searchInput->setStyleSheet(
      "QLineEdit { background: #1e293b; color: white; padding: 10px; "
      "border-radius: 8px; border: 1px solid #334155; }");
  connect(m_searchInput, &QLineEdit::textChanged, this,
          &AdminPanel::onSearchChanged);
  mainLayout->addWidget(m_searchInput);

  // Table
  m_table = new QTableWidget();
  m_table->setColumnCount(8);
  m_table->setHorizontalHeaderLabels({"Key", "Name", "Role", "Status", "HWID",
                                      "IP", "Last Active", "Created/Exp"});
  m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setContextMenuPolicy(Qt::CustomContextMenu);
  m_table->setStyleSheet(
      "QTableWidget { background: #0f172a; border: 1px solid #334155; "
      "border-radius: 8px; gridline-color: #334155; color: #cbd5e1; }"
      "QHeaderView::section { background: #1e293b; color: white; padding: 8px; "
      "border: none; font-weight: bold; }"
      "QTableWidget::item { padding: 5px; }"
      "QTableWidget::item:selected { background: " +
      Utils::ACCENT_COLOR + "; color: white; }");
  connect(m_table, &QTableWidget::customContextMenuRequested, this,
          &AdminPanel::showContextMenu);
  mainLayout->addWidget(m_table);
}

void AdminPanel::refreshData() {
  m_refreshBtn->setEnabled(false);
  m_refreshBtn->setText("LOADING...");
  m_authManager->fetchAllUsers();
}

void AdminPanel::onUsersFetched(const QList<QJsonObject> &users) {
  m_allUsers = users;
  filterTable();
  m_refreshBtn->setEnabled(true);
  m_refreshBtn->setText("🔄 REFRESH");
}

void AdminPanel::filterTable() {
  m_table->setRowCount(0);
  QString query = m_searchInput->text().toLower();

  for (const auto &user : m_allUsers) {
    QString key = user["key"].toString();
    QString name = user["name"].toString();
    QString role = user["role"].toString();
    QString status = user["status"].toString();

    if (query.isEmpty() || key.contains(query) ||
        name.toLower().contains(query) || role.toLower().contains(query)) {
      int row = m_table->rowCount();
      m_table->insertRow(row);

      m_table->setItem(row, 0, new QTableWidgetItem(key));
      m_table->setItem(row, 1, new QTableWidgetItem(name));

      QTableWidgetItem *roleItem = new QTableWidgetItem(role.toUpper());
      if (role == "admin" || role.contains("dev"))
        roleItem->setForeground(QBrush(QColor("#a855f7"))); // Purple
      else if (role == "pro")
        roleItem->setForeground(QBrush(QColor("#facc15"))); // Gold
      m_table->setItem(row, 2, roleItem);

      QTableWidgetItem *statusItem = new QTableWidgetItem(status.toUpper());
      if (status == "banned")
        statusItem->setForeground(QBrush(QColor("#ef4444"))); // Red
      else if (status == "active")
        statusItem->setForeground(QBrush(QColor("#22c55e"))); // Green
      m_table->setItem(row, 3, statusItem);

      m_table->setItem(row, 4, new QTableWidgetItem(user["hwid"].toString()));

      QString ip = user["last_ip"].toString();
      if (ip.isEmpty())
        ip = user["ip"].toString();
      m_table->setItem(row, 5, new QTableWidgetItem(ip));

      m_table->setItem(row, 6,
                       new QTableWidgetItem(user["last_login"].toString()));
      m_table->setItem(row, 7, new QTableWidgetItem(user["expiry"].toString()));
    }
  }
}

void AdminPanel::onSearchChanged(const QString &) { filterTable(); }

void AdminPanel::showContextMenu(const QPoint &pos) {
  QTableWidgetItem *item = m_table->itemAt(pos);
  if (!item)
    return;

  int row = item->row();
  m_selectedKey = m_table->item(row, 0)->text();
  QString currentStatus = m_table->item(row, 3)->text();

  QMenu menu(this);
  menu.addAction("📋 Copy Key", this, &AdminPanel::copyKey);
  menu.addSeparator();

  if (currentStatus == "BANNED") {
    menu.addAction("✅ Unban User", this, &AdminPanel::unbanUser);
  } else {
    menu.addAction("🚫 Ban User", this, &AdminPanel::banUser);
  }

  menu.addAction("🔄 Reset HWID", this, &AdminPanel::resetHWID);
  menu.addAction("👑 Set Role...", this, &AdminPanel::setRole);
  menu.addAction("✏️ Rename User...", this, &AdminPanel::renameUser);
  menu.addSeparator();
  menu.addAction("⚠️ Force Update", this, &AdminPanel::forceUpdateUser);

  menu.exec(m_table->viewport()->mapToGlobal(pos));
}

void AdminPanel::copyKey() {
  QApplication::clipboard()->setText(m_selectedKey);
}

void AdminPanel::banUser() {
  if (QMessageBox::question(this, "Confirm Ban", "Ban this user?") ==
      QMessageBox::Yes) {
    QJsonObject updates;
    updates["status"] = "banned";
    m_authManager->updateUser(m_selectedKey, updates);
  }
}

void AdminPanel::unbanUser() {
  QJsonObject updates;
  updates["status"] = "active";
  m_authManager->updateUser(m_selectedKey, updates);
}

void AdminPanel::resetHWID() {
  if (QMessageBox::question(this, "Reset HWID", "Clear HWID binding?") ==
      QMessageBox::Yes) {
    QJsonObject updates;
    updates["hwid"] = "";
    m_authManager->updateUser(m_selectedKey, updates);
  }
}

void AdminPanel::setRole() {
  bool ok;
  QString role = QInputDialog::getText(
      this, "Set Role", "New Role (e.g. pro, admin, VIP):", QLineEdit::Normal,
      "", &ok);
  if (ok && !role.isEmpty()) {
    QJsonObject updates;
    updates["role"] = role;
    m_authManager->updateUser(m_selectedKey, updates);
  }
}

void AdminPanel::renameUser() {
  bool ok;
  QString name = QInputDialog::getText(this, "Rename User",
                                       "New Name:", QLineEdit::Normal, "", &ok);
  if (ok && !name.isEmpty()) {
    QJsonObject updates;
    updates["name"] = name;
    m_authManager->updateUser(m_selectedKey, updates);
  }
}

void AdminPanel::forceUpdateUser() {
  if (QMessageBox::question(this, "Force Update",
                            "Force this user to update app?") ==
      QMessageBox::Yes) {
    QJsonObject updates;
    updates["force_update"] = true;
    m_authManager->updateUser(m_selectedKey, updates);
  }
}

} // namespace GOL
