
#include "SettingsPage.h"
#include "Logger.h"
#include "Utils.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QtCore/QSettings>

namespace GOL {

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent) {
  setupUI();
  loadSettings();
}

void SettingsPage::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(50, 40, 50, 40);
  mainLayout->setSpacing(30);

  // --- Header ---
  QLabel *title = new QLabel("SETTINGS & PREFERENCES");
  title->setStyleSheet(
      "color: white; font-size: 28px; font-weight: 900; letter-spacing: 1px;");
  mainLayout->addWidget(title);

  // --- Appearance Section ---
  QWidget *appearanceGroup = new QWidget();
  QVBoxLayout *appLayout = new QVBoxLayout(appearanceGroup);
  appLayout->setSpacing(10);

  QLabel *appHeader = new QLabel("🎨 APPEARANCE");
  appHeader->setStyleSheet("color: #94a3b8; font-size: 14px; font-weight: "
                           "bold; margin-bottom: 5px;");
  appLayout->addWidget(appHeader);

  m_themeCombo = new QComboBox();
  m_themeCombo->addItems({"Sapphire (Default)", "Dark Mode", "Light Mode"});
  m_themeCombo->setStyleSheet(
      "QComboBox { background: rgba(255,255,255,0.05); color: white; border: "
      "1px solid rgba(255,255,255,0.1); padding: 8px; border-radius: 6px; }"
      "QComboBox::drop-down { border: none; }"
      "QComboBox QAbstractItemView { background: #1e293b; color: white; "
      "selection-background-color: #3b82f6; }");
  connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SettingsPage::onThemeChanged);
  appLayout->addWidget(m_themeCombo);

  mainLayout->addWidget(appearanceGroup);

  // --- Data Section ---
  QWidget *dataGroup = new QWidget();
  QVBoxLayout *dataLayout = new QVBoxLayout(dataGroup);
  dataLayout->setSpacing(10);

  QLabel *dataHeader = new QLabel("📂 DATA STORAGE");
  dataHeader->setStyleSheet("color: #94a3b8; font-size: 14px; font-weight: "
                            "bold; margin-bottom: 5px;");
  dataLayout->addWidget(dataHeader);

  QLabel *pathDesc = new QLabel("Default Export Directory:");
  pathDesc->setStyleSheet("color: #64748b; font-size: 12px;");
  dataLayout->addWidget(pathDesc);

  QHBoxLayout *pathBox = new QHBoxLayout();
  m_pathInput = new QLineEdit();
  m_pathInput->setReadOnly(true);
  m_pathInput->setPlaceholderText("Select a directory...");
  m_pathInput->setStyleSheet(
      "background: rgba(0,0,0,0.2); color: #cbd5e1; border: 1px solid "
      "rgba(255,255,255,0.1); padding: 8px; border-radius: 6px;");

  QPushButton *browseBtn = new QPushButton("Browse");
  browseBtn->setCursor(Qt::PointingHandCursor);
  browseBtn->setStyleSheet(
      "background: #3b82f6; color: white; font-weight: bold; padding: 8px "
      "16px; border-radius: 6px; border: none;");
  connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::onBrowsePath);

  pathBox->addWidget(m_pathInput);
  pathBox->addWidget(browseBtn);
  dataLayout->addLayout(pathBox);

  mainLayout->addWidget(dataGroup);

  // --- System Section ---
  // (Removed Clear Cache as per user request)
  mainLayout->addStretch();
}

void SettingsPage::loadSettings() {
  QSettings settings("GOL", "EventsPro");

  // Theme
  QString theme = settings.value("theme", "Sapphire (Default)").toString();
  m_themeCombo->setCurrentText(theme);

  // Path
  QString path =
      settings
          .value("exportPath", QStandardPaths::writableLocation(
                                   QStandardPaths::DocumentsLocation))
          .toString();
  m_pathInput->setText(path);
}

void SettingsPage::onBrowsePath() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Default Export Directory", m_pathInput->text());
  if (!dir.isEmpty()) {
    m_pathInput->setText(dir);
    QSettings settings("GOL", "EventsPro");
    settings.setValue("exportPath", dir);
    Logger::instance().log("Default export path updated: " + dir);
    emit defaultPathChanged(dir);
  }
}

void SettingsPage::onThemeChanged(int index) {
  QString theme = m_themeCombo->currentText();
  QSettings settings("GOL", "EventsPro");
  settings.setValue("theme", theme);
  Logger::instance().log("Theme changed to: " + theme);

  // Placeholder for real theme application
  // In a full implementation, this would load style_dark.qss or style_light.qss
  QApplication *app =
      qobject_cast<QApplication *>(QCoreApplication::instance());
  if (app) {
    if (theme.contains("Light")) {
      // Example: Light Mode Override (Simple)
      app->setPalette(QPalette(Qt::white));
      // Reload QSS if needed or clear it
      app->setStyleSheet("");
    } else {
      // Reload Default QSS
      QFile file(Utils::resourcePath("resources/style.qss"));
      if (file.open(QFile::ReadOnly)) {
        app->setStyleSheet(QLatin1String(file.readAll()));
      }
    }
  }
  emit themeChanged(theme);
}

} // namespace GOL
