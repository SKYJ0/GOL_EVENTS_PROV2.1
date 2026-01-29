#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "AdminPanel.h"
#include "AuthManager.h"
#include "GuideDialog.h"
#include "ToolCard.h"
#include "UpdateChecker.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace GOL {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void onAuthSuccess();
  void onAuthFailed(const QString &reason);
  void processLogin();
  void showHome();
  void showAnalytics();
  void showDataProcessing();
  void showCreativeTools();
  void showSettings();
  void showAdmin();
  void launchTool(const QString &toolName);
  void showGuide(const QString &toolName, const QString &displayName);
  void onUpdateAvailable(const QString &version, const QString &url);
  void setSidebarCompact(bool compact);
  void onAvatarClicked();

private:
  void setupUI();
  void showSplash();
  void setupLoginUI();
  void setupSidebar();
  void setupHomeUI();
  void setupAnalyticsUI();
  void setupDataProcessingUI();
  void setupCreativeToolsUI();

  ToolCard *createToolCard(const QString &title, const QString &desc,
                           const QString &scriptName, bool isPro = false);
  QWidget *createStatCard(const QString &title, const QString &value,
                          const QString &color, const QString &icon);
  void updateNavStyles(QPushButton *activeBtn);
  void switchToPage(QWidget *newPage);
  void setupProfileCard();

  void applyDarkTheme();
  void animateLoading(double progress);
  void resizeEvent(QResizeEvent *event) override;
  // Native event handler for robust resizing on Windows
  bool nativeEvent(const QByteArray &eventType, void *message,
                   qintptr *result) override;
  // Manual mouse events removed in favor of native handling
  // void mousePressEvent(QMouseEvent *event) override;
  // void mouseMoveEvent(QMouseEvent *event) override;

  // Main components
  AuthManager *m_authManager;
  UpdateChecker *m_updateChecker;

  // UI Elements
  QStackedWidget *m_stackedWidget;
  QWidget *m_splashWidget;
  QWidget *m_loginWidget;
  QWidget *m_mainWidget;
  QWidget *m_homeWidget;
  QWidget *m_analyticsWidget;
  QWidget *m_dataWidget;
  QWidget *m_creativeWidget;
  QWidget *m_adminWidget;
  QWidget *m_sidebarWidget;
  QFrame *m_profileCard;
  QFrame *m_navIndicator;

  // Login elements
  QLineEdit *m_keyInput;
  QLabel *m_msgLabel;

  // Splash elements
  QProgressBar *m_loadBar;
  QLabel *m_splashMsg;

  // Navigation buttons
  QPushButton *m_homeBtn;
  QPushButton *m_analyticsBtn;
  QPushButton *m_dataBtn;
  QPushButton *m_creativeBtn;
  QPushButton *m_adminBtn;
  QPushButton *m_settingsBtn;

  // Log box
  QTextEdit *m_logBox;

  // Current state
  QString m_currentUserName;
  QString m_currentRole;
  int m_currentCredits;
  bool m_isSidebarCompact = false;

  // Sidebar Sub-elements
  QLabel *m_sidebarTitle;
  QLabel *m_sidebarSubtitle;
  QWidget *m_profileDetails;
  QLabel *m_nameLabel;
  QLabel *m_roleLabel;
  QPushButton *m_avatarBtn;
  QWidget *m_sidebarHeader;
  QPoint m_dragPos;
  bool m_isResizing = false;
  Qt::Edges m_resizeEdge = Qt::Edges();

  void updateCursorShape(const QPoint &pos);

  bool isUserPro() const {
    QString r = m_currentRole.toLower().trimmed();
    return r.contains("pro") || r.contains("admin") || r.contains("dev") ||
           r.contains("vip");
  }
};

} // namespace GOL

#endif // MAINWINDOW_H
