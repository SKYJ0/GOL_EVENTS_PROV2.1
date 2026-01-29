#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QSettings>
#include <QWidget>
#include <QtGlobal>

class QComboBox;
class QLineEdit;
class QPushButton;

namespace GOL {

class SettingsPage : public QWidget {
  Q_OBJECT

public:
  explicit SettingsPage(QWidget *parent = nullptr);

signals:
  void themeChanged(const QString &themeName);
  void defaultPathChanged(const QString &newPath);

private slots:
  void onBrowsePath();
  void onThemeChanged(int index);

private:
  void setupUI();
  void loadSettings();

  QComboBox *m_themeCombo;
  QLineEdit *m_pathInput;
};

} // namespace GOL

#endif // SETTINGSPAGE_H
