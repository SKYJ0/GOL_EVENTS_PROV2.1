#include "DailyReport.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>
#include <utility>
#include <vector>

namespace GOL {

DailyReport::DailyReport(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("DAILY REPORT BUILDER");
  resize(1100, 700);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));

  setupUI();
  loadMatches();
  loadSettings();
  generateReport(); // Initial empty report
}

void DailyReport::setupUI() {
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(20);

  // --- LEFT COLUMN: ACTIONS ---
  QScrollArea *leftScroll = new QScrollArea();
  leftScroll->setWidgetResizable(true);
  leftScroll->setStyleSheet("background: transparent; border: none;");

  QWidget *leftWidget = new QWidget();
  QVBoxLayout *leftLayoutActions = new QVBoxLayout(leftWidget);
  leftLayoutActions->setAlignment(Qt::AlignTop);
  leftLayoutActions->setSpacing(20);
  leftScroll->setWidget(leftWidget);

  QLabel *lblActions = new QLabel("1. SELECT TASK"); // Renamed
  lblActions->setStyleSheet("font-weight: bold; color: #fbbf24; font-size: "
                            "14px; margin-bottom: 5px;");
  leftLayoutActions->addWidget(lblActions);

  // Update createCategory to accept a default section for custom tasks
  auto createCategory =
      [&](const QString &title, const QString &color,
          const std::vector<std::pair<QString, QString>> &actions,
          const QString &defaultSection) {
        QGroupBox *group = new QGroupBox(title);
        group->setStyleSheet(
            QString("QGroupBox { border: 1px solid #444; border-radius: 5px; "
                    "margin-top: 10px; font-weight: bold; color: %1; }"
                    "QGroupBox::title { subcontrol-origin: margin; left: 10px; "
                    "padding: 0 3px; }")
                .arg(color));
        QVBoxLayout *gl = new QVBoxLayout(group);
        gl->setSpacing(5);

        for (const auto &pair : actions) {
          QString name = pair.first;
          QString section = pair.second;
          QPushButton *btn = new QPushButton(name);
          btn->setCursor(Qt::PointingHandCursor);
          btn->setStyleSheet(
              QString("QPushButton { background: #333; color: #e5e5e5; border: "
                      "1px solid #555; border-radius: 4px; padding: 6px; "
                      "text-align: left; }"
                      "QPushButton:hover { background: %1; color: black; "
                      "border-color: %1; }")
                  .arg(color));
          connect(btn, &QPushButton::clicked,
                  [this, name, section]() { onActionClicked(name, section); });
          gl->addWidget(btn);
        }

        // Add "Custom Task" button for ALL categories
        QPushButton *btnCustom = new QPushButton("+ Add Custom Task");
        btnCustom->setCursor(Qt::PointingHandCursor);
        btnCustom->setStyleSheet(
            "QPushButton { background: #333; color: #fbbf24; border: 1px "
            "dashed #fbbf24; border-radius: 4px; padding: 6px; text-align: "
            "center; } QPushButton:hover { background: #444; }");

        // Connect to addCustomTask but we need to know the default section
        // context. We'll overload addCustomTask or use a lambda capturing the
        // default section.
        connect(btnCustom, &QPushButton::clicked, [this, defaultSection]() {
          bool ok;
          QString text = QInputDialog::getText(
              this, "Add Custom Task", "Task Description:", QLineEdit::Normal,
              "", &ok);
          if (ok && !text.isEmpty()) {
            onActionClicked(text, defaultSection);
          }
        });
        gl->addWidget(btnCustom);

        leftLayoutActions->addWidget(group);
      };

  // PLATFORMS
  createCategory("PLATFORMS", "#38bdf8",
                 {{"Registering Orders", "1️⃣ Tickets / Orders handled"},
                  {"Check Listing", "3️⃣ Listing / Stock checks"},
                  {"Check Prices", "3️⃣ Listing / Stock checks"},
                  {"Delivery", "1️⃣ Tickets / Orders handled"},
                  {"Check Info", "4️⃣ Disputes / Issues"},
                  {"Contact Support", "4️⃣ Disputes / Issues"}},
                 "1️⃣ Tickets / Orders handled");

  // PDF
  createCategory("PDF TASKS", "#a78bfa",
                 {{"Ticket To Do", "2️⃣ Name changes / PDFs"},
                  {"Change Name", "2️⃣ Name changes / PDFs"},
                  {"Move Tickets", "2️⃣ Name changes / PDFs"},
                  {"Create Placeholder", "2️⃣ Name changes / PDFs"},
                  {"Download Tickets", "2️⃣ Name changes / PDFs"},
                  {"Verifying Ticket To Do", "2️⃣ Name changes / PDFs"}},
                 "2️⃣ Name changes / PDFs");

  // OTHER
  createCategory("OTHER", "#10b981",
                 {{"Buying Tickets", "1️⃣ Tickets / Orders handled"},
                  {"Teaching Colleague", "4️⃣ Disputes / Issues"}},
                 "4️⃣ Disputes / Issues");

  leftLayoutActions->addStretch(); // Changed from centerLayout

  // --- CENTER COLUMN: MATCHES ---
  QVBoxLayout *centerLayoutMatches = new QVBoxLayout();

  // Header Name Input
  QLabel *lblHeader = new QLabel("2. SELECT MATCH & STATUS"); // Renamed
  lblHeader->setStyleSheet(
      "font-weight: bold; color: #38bdf8; font-size: 14px;");
  centerLayoutMatches->addWidget(lblHeader);

  m_nameInput = new QLineEdit();
  m_nameInput->setPlaceholderText("Enter Your Name"); // Updated placeholder
  m_nameInput->setStyleSheet(
      "QLineEdit { background: #333; color: white; border: 1px solid #555; "
      "padding: 5px; border-radius: 5px; }"); // Updated padding
  connect(m_nameInput, &QLineEdit::editingFinished, this,
          &DailyReport::saveSettings);
  connect(m_nameInput, &QLineEdit::textChanged,
          [this](const QString &) { generateReport(); });
  centerLayoutMatches->addWidget(m_nameInput);

  // Status Checkbox
  m_chkPending = new QCheckBox("⚠️ Mark as Pending / Issue");
  m_chkPending->setStyleSheet(
      "QCheckBox { color: #ef4444; font-weight: bold; font-size: 13px; } "
      "QCheckBox::indicator { width: 18px; height: 18px; }");
  centerLayoutMatches->addWidget(m_chkPending);

  centerLayoutMatches->addSpacing(10);

  QLabel *lblList = new QLabel("Active Matches / Platforms:");
  lblList->setStyleSheet("color: #cbd5e1;");
  centerLayoutMatches->addWidget(lblList);

  m_matchesList = new QListWidget();
  m_matchesList->setStyleSheet(
      "QListWidget { background: #0f172a; border: 1px solid #334155; "
      "border-radius: 8px; font-size: 13px; }"
      "QListWidget::item { padding: 8px; }"
      "QListWidget::item:selected { background: #38bdf8; color: black; }");
  centerLayoutMatches->addWidget(m_matchesList);

  QHBoxLayout *matchBtnLayout = new QHBoxLayout();

  QPushButton *btnAdd = new QPushButton("+ Match");
  btnAdd->setStyleSheet("background: #10b981; color: white; font-weight: bold; "
                        "border-radius: 5px; padding: 5px;");
  connect(btnAdd, &QPushButton::clicked, this, &DailyReport::addMatch);

  QPushButton *btnAddPlatform = new QPushButton("+ Platforms");
  btnAddPlatform->setStyleSheet(
      "background: #f59e0b; color: white; font-weight: bold; "
      "border-radius: 5px; padding: 5px;");
  connect(btnAddPlatform, &QPushButton::clicked, [this]() {
    QStringList platforms = {"VIAGOGO", "TIXSTOCK", "TICKETNET"};
    for (const auto &p : platforms) {
      // Check if already exists to avoid duplicates
      if (m_matchesList->findItems(p, Qt::MatchExactly).isEmpty()) {
        m_matchesList->addItem(p);
      }
    }
  });

  QPushButton *btnDel = new QPushButton("🗑️");
  btnDel->setFixedWidth(30);
  btnDel->setStyleSheet("background: #ef4444; color: white; font-weight: bold; "
                        "border-radius: 5px; padding: 5px;");
  connect(btnDel, &QPushButton::clicked, this, &DailyReport::removeMatch);

  matchBtnLayout->addWidget(btnAdd);
  matchBtnLayout->addWidget(btnAddPlatform);
  matchBtnLayout->addWidget(btnDel);
  centerLayoutMatches->addLayout(matchBtnLayout);

  // --- RIGHT COLUMN: PREVIEW ---
  QVBoxLayout *rightLayout = new QVBoxLayout();

  QLabel *lblPreview = new QLabel("3. FINAL REPORT");
  lblPreview->setStyleSheet(
      "font-weight: bold; color: #a78bfa; font-size: 14px;");
  rightLayout->addWidget(lblPreview);

  m_previewArea = new QTextEdit();
  m_previewArea->setReadOnly(true);
  m_previewArea->setFont(QFont("Consolas", 11));
  m_previewArea->setStyleSheet("background: #1e1e1e; color: #d4d4d4; border: "
                               "1px solid #333; padding: 10px;");
  rightLayout->addWidget(m_previewArea);

  QHBoxLayout *actionBtns = new QHBoxLayout();

  QPushButton *btnClearLog = new QPushButton("🗑️ CLEAR");
  btnClearLog->setFixedHeight(40);
  btnClearLog->setFixedWidth(80);
  btnClearLog->setCursor(Qt::PointingHandCursor);
  btnClearLog->setStyleSheet("QPushButton { background: #b91c1c; color: white; "
                             "border-radius: 5px; font-weight: bold; } "
                             "QPushButton:hover { background: #ef4444; }");
  connect(btnClearLog, &QPushButton::clicked, [this]() {
    // Only clear report content, not name
    m_reportData.clear();
    generateReport();
  });

  QPushButton *btnCopy = new QPushButton("📋 COPY TO CLIPBOARD");
  btnCopy->setFixedHeight(40);
  btnCopy->setCursor(Qt::PointingHandCursor);
  btnCopy->setStyleSheet(
      "QPushButton { background: #6366f1; color: white; font-weight: "
      "bold; font-size: 14px; border-radius: 5px; }"
      "QPushButton:hover { background: #818cf8; }");
  connect(btnCopy, &QPushButton::clicked, this, &DailyReport::copyToClipboard);

  actionBtns->addWidget(btnClearLog);
  actionBtns->addWidget(btnCopy);

  rightLayout->addLayout(actionBtns);

  // Assembly
  mainLayout->addWidget(leftScroll, 30);
  mainLayout->addLayout(centerLayoutMatches, 25);
  mainLayout->addLayout(rightLayout, 45);
}

void DailyReport::addMatch() {
  QDialog dlg(this);
  dlg.setWindowTitle("Add Match");
  dlg.setStyleSheet(
      QString("background: %1; color: white;").arg(Utils::BG_COLOR));
  QVBoxLayout l(&dlg);

  QLabel *lbl = new QLabel("Select Home Team (or type custom):");
  l.addWidget(lbl);

  QComboBox *comboHome = new QComboBox();
  comboHome->setEditable(true);
  comboHome->addItems(SERIE_A_TEAMS);
  comboHome->setStyleSheet(
      "QComboBox { background: #333; color: white; padding: 5px; } QComboBox "
      "QAbstractItemView { background: #333; color: white; "
      "selection-background-color: #38bdf8; }");
  comboHome->setCurrentIndex(-1); // No default selection
  comboHome->setPlaceholderText("Home Team");
  l.addWidget(comboHome);

  QLabel *lblVs = new QLabel("vs");
  lblVs->setAlignment(Qt::AlignCenter);
  l.addWidget(lblVs);

  QLabel *lblAway = new QLabel("Select Opponent (or type custom):");
  l.addWidget(lblAway);

  QComboBox *comboAway = new QComboBox();
  comboAway->setEditable(true);
  comboAway->addItems(SERIE_A_TEAMS);
  comboAway->setStyleSheet(
      "QComboBox { background: #333; color: white; padding: 5px; } QComboBox "
      "QAbstractItemView { background: #333; color: white; "
      "selection-background-color: #38bdf8; }");
  comboAway->setCurrentIndex(-1);
  comboAway->setPlaceholderText("Away Team");
  l.addWidget(comboAway);

  QPushButton *btnOk = new QPushButton("Add");
  btnOk->setStyleSheet("background: #10b981; color: white; padding: 8px; "
                       "font-weight: bold; border-radius: 4px;");
  connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
  l.addWidget(btnOk);

  if (dlg.exec() == QDialog::Accepted) {
    QString home = comboHome->currentText().trimmed();
    QString away = comboAway->currentText().trimmed();

    if (!home.isEmpty() && !away.isEmpty()) {
      QString matchName = QString("%1 vs %2").arg(home, away);
      m_matchesList->addItem(matchName);
      saveMatches();
    }
  }
}

void DailyReport::removeMatch() {
  QListWidgetItem *item = m_matchesList->currentItem();
  if (item) {
    delete item;
    saveMatches();
  }
}

QString DailyReport::getSelectedMatch() {
  QListWidgetItem *item = m_matchesList->currentItem();
  return item ? item->text() : "General";
}

// Update format to "Status : Match - Action" or "Action : Match [Status]"
// User requested: "for each task make it as a title and for status make it two
// complete o lokhra mazal process" Interpretation: Task Name (Title) - Match
// Name - Status

// Helper to get icon for action
QString getActionIcon(const QString &action) {
  if (action.contains("Registering"))
    return "🎟️";
  if (action.contains("Delivery"))
    return "✅";
  if (action.contains("Buying"))
    return "🛒";
  if (action.contains("Listing") || action.contains("Prices"))
    return "📊";
  if (action.contains("Ticket To Do") || action.contains("Name"))
    return "📝";
  if (action.contains("Move") || action.contains("Placeholder"))
    return "🔄";
  if (action.contains("Download"))
    return "⬇️";
  if (action.contains("Support") || action.contains("Teaching") ||
      action.contains("Info"))
    return "💬";
  return "🔹"; // Default
}

// Force Rebuild Tag: v7 - Validation
void DailyReport::onActionClicked(const QString &actionName,
                                  const QString & /*section_unused*/) {
  if (!m_matchesList->currentItem()) {
    QMessageBox::warning(this, "Select Match", "Please select a match first!");
    return;
  }

  QString match = getSelectedMatch();
  bool isPending = m_chkPending->isChecked();

  // Status suffix only if pending
  QString suffix = isPending ? " (In progress)" : "";

  // Entry is just the match name + suffix
  QString entry = match + suffix;

  // Store under the ACTION NAME as the key
  m_reportData[actionName].append(entry);

  generateReport();
}

void DailyReport::generateReport() {
  QString report;
  QString dateStr = QDateTime::currentDateTime().toString("dd/MM/yyyy");
  QString name = m_nameInput->text().trimmed();
  if (name.isEmpty())
    name = "[Your Name]";

  // Nice Header (3inwan zwin)
  report += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  report += "   🚀 DAILY ACTIVITY REPORT   \n";
  report += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  report += QString("👤 Name: %1\n").arg(name);
  report += QString("📅 Date: %1\n\n").arg(dateStr);

  QMapIterator<QString, QStringList> i(m_reportData);
  while (i.hasNext()) {
    i.next();
    QString action = i.key();
    const QStringList &matches = i.value();

    if (matches.isEmpty())
      continue;

    // Add Icon to Section Title
    QString icon = getActionIcon(action);

    // Section Title: === Icon Action ===
    report += QString("=== %1 %2 ===\n\n").arg(icon, action);

    // Matches
    for (const QString &match : matches) {
      report += match + "\n";
    }
    report += "\n"; // Spacer between sections
  }

  m_previewArea->setPlainText(report);
}

void DailyReport::copyToClipboard() {
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(m_previewArea->toPlainText());
  QMessageBox::information(this, "Copied", "Report copied to clipboard!");
}

void DailyReport::saveMatches() {
  QStringList items;
  for (int i = 0; i < m_matchesList->count(); ++i) {
    items << m_matchesList->item(i)->text();
  }

  QSettings settings("GOL", "DailyReport");
  settings.setValue("matches", items);
}

void DailyReport::loadMatches() {
  QSettings settings("GOL", "DailyReport");
  QStringList items = settings.value("matches").toStringList();

  m_matchesList->clear();
  for (const QString &item : items) {
    m_matchesList->addItem(item);
  }
}

void DailyReport::addCustomTask() {
  bool ok;
  QString text = QInputDialog::getText(
      this, "Add Custom Task", "Task Description:", QLineEdit::Normal, "", &ok);
  if (ok && !text.isEmpty()) {
    // Map custom tasks to "4. Disputes / Issues" as a catch-all for "Other"
    // work
    onActionClicked(text, "4️⃣ Disputes / Issues");
  }
}

void DailyReport::saveSettings() {
  QSettings settings("GOL", "DailyReport");
  settings.setValue("userName", m_nameInput->text());
}

void DailyReport::loadSettings() {
  QSettings settings("GOL", "DailyReport");
  QString name = settings.value("userName").toString();
  m_nameInput->setText(name);
}

} // namespace GOL
