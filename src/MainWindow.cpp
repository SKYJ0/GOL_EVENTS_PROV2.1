
#include "MainWindow.h"
#include "AnimatedBackground.h"
#include "CosmicDialog.h"
#include "FlowLayout.h"
#include "Logger.h"
#include "SettingsPage.h"
#include "ToolCard.h"
#include "Utils.h"
#include "tools/CalcStock.h"
#include "tools/CheckListing.h"
#include "tools/CheckPrice.h"
#include "tools/DailyReport.h"
#include "tools/ExpanderSeats.h"
#include "tools/PdfsToTxt.h"
#include "tools/Placeholder.h"
#include "tools/QrGenerator.h"
#include "tools/RenamerFv.h"
#include "tools/SplitterRenamer.h"
#include "tools/StockReport.h"
#include "tools/VerifyOrders.h"
#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QMessageBox>
#include <QMouseEvent>
// Add these overrides at end of constructor or in file (but needs class scope,
// wait. I should add these methods to MainWindow.cpp. I'll search for
// resizeEvent which is likely there.)
#include <QDesktopServices>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QProcess>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSplitter>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace GOL {

// Helper Class for Pulse Animation
class AnimatedPlaceholder : public QWidget {
public:
  AnimatedPlaceholder(QWidget *parent = nullptr)
      : QWidget(parent), m_offset(0.0f) {
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
      m_offset += 0.02f;
      if (m_offset > 1.0f)
        m_offset = 0.0f;
      update();
    });
    timer->start(30); // ~30fps
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Clip to rounded rect (top corners 16px)
    QPainterPath path;
    path.addRoundedRect(rect(), 16, 16);
    // We really only want top corners rounded, but full rounded is okay if
    // bottom is covered or if we draw specifically. Actually, let's just fill
    // rect, container handles generic rounding via masking if needed, but here
    // we are a child widget. Let's just draw specific path.

    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0, QColor(255, 255, 255, 5));

    // Moving highlight
    float pos = m_offset;
    gradient.setColorAt(pos, QColor(255, 255, 255, 20)); // Highlight

    gradient.setColorAt(1, QColor(255, 255, 255, 5));

    p.fillRect(rect(), QColor(0, 0, 0, 80)); // Base dark bg
    p.fillRect(rect(), gradient);

    // Text
    p.setPen(QColor(255, 255, 255, 100));
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    p.drawText(rect(), Qt::AlignCenter, "▶ PREVIEW");
  }

private:
  float m_offset;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_authManager(new AuthManager(this)),
      m_updateChecker(new UpdateChecker(this)), m_currentCredits(0),
      m_logBox(nullptr), m_splashWidget(nullptr), m_loginWidget(nullptr),
      m_mainWidget(nullptr), m_homeWidget(nullptr), m_analyticsWidget(nullptr),
      m_dataWidget(nullptr), m_creativeWidget(nullptr),
      m_sidebarWidget(nullptr), m_profileCard(nullptr), m_navIndicator(nullptr),
      m_keyInput(nullptr), m_msgLabel(nullptr), m_loadBar(nullptr),
      m_splashMsg(nullptr), m_homeBtn(nullptr), m_analyticsBtn(nullptr),
      m_dataBtn(nullptr), m_creativeBtn(nullptr), m_adminBtn(nullptr),
      m_adminWidget(nullptr) {
  setWindowIcon(QIcon(Utils::resourcePath("resources/knight.png")));
  setWindowTitle(
      QString("GOLEVENTS PRO %1 (LOCKED BUILD)").arg(Utils::VERSION));
  resize(450, 550);         // Initial size small for Login Card
  setMinimumSize(400, 500); // Allow resizing to phone/tablet aspect
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setAttribute(Qt::WA_TranslucentBackground);
  setMouseTracking(true); // Enable mouse tracking for resize cursor

  // Load Global Stylesheet
  QFile file(Utils::resourcePath("resources/style.qss"));
  if (file.open(QFile::ReadOnly)) {
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
  } else {
    // Fallback or log if missing
    Utils::logToFile("Warning: Could not load resources/style.qss");
    applyDarkTheme(); // Keep as fallback if desired, or remove
  }
  setupUI();

  // Connect auth signals
  connect(m_authManager, &AuthManager::authenticationSuccess, this,
          &MainWindow::onAuthSuccess);
  connect(m_authManager, &AuthManager::authenticationFailed, this,
          &MainWindow::onAuthFailed);

  // Connect update signals
  connect(m_updateChecker, &UpdateChecker::updateAvailable, this,
          &MainWindow::onUpdateAvailable);

  // Connect Logger
  connect(&Logger::instance(), &Logger::logAdded, this,
          [this](const QString &msg) {
            if (m_logBox) {
              m_logBox->append(msg);
            }
          });

  showSplash();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
  // Use AnimatedBackground as the central container
  AnimatedBackground *bg = new AnimatedBackground(this);
  setCentralWidget(bg);

  // Change to QVBoxLayout for Unified Header (Top) + Body (Bottom)
  QVBoxLayout *mainLayout = new QVBoxLayout(bg);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // --- TOP HEADER (Unified Title Bar) ---
  QWidget *topHeader = new QWidget();
  topHeader->setObjectName("header"); // CRITICAL: For nativeEvent hit testing
  topHeader->setFixedHeight(40);      // Slimmer generic title bar
  topHeader->setStyleSheet(
      "QWidget#header { background: #0f172a; border-bottom: 1px solid "
      "rgba(255,255,255,0.05); }"); // Explicit dark bg
  mainLayout->addWidget(topHeader);

  // Top Header Layout (Controls added in next block)

  QHBoxLayout *headerLayout = new QHBoxLayout(topHeader);
  headerLayout->setContentsMargins(20, 0, 10, 0); // Spacing for controls
  headerLayout->setSpacing(10);

  // Title / Drag Area
  // We can add a small app title here if desired, or just spacing
  headerLayout->addStretch();

  // Separator
  QFrame *sep = new QFrame();
  sep->setFixedSize(1, 24);
  sep->setStyleSheet("background: rgba(255,255,255,0.2);");
  headerLayout->addWidget(sep);

  // Window Controls (Min, Max, Close)
  auto createWindowBtn = [](const QString &text,
                            const QString &hoverColor) -> QPushButton * {
    QPushButton *btn = new QPushButton(text);
    btn->setFixedSize(40, 30);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        QString("QPushButton { background: transparent; color: #cbd5e1; "
                "font-size: 14px; border: none; }"
                "QPushButton:hover { background: %1; color: white; }")
            .arg(hoverColor));
    return btn;
  };

  QPushButton *btnMin = createWindowBtn("─", "rgba(255, 255, 255, 0.1)");
  connect(btnMin, &QPushButton::clicked, this, &MainWindow::showMinimized);

  QPushButton *btnMax = createWindowBtn("⬜", "rgba(255, 255, 255, 0.1)");
  connect(btnMax, &QPushButton::clicked, [this, btnMax]() {
    if (isMaximized()) {
      showNormal();
      btnMax->setText("⬜");
    } else {
      showMaximized();
      btnMax->setText("❐");
    }
  });

  QPushButton *btnClose = createWindowBtn("✕", "#ef4444");
  connect(btnClose, &QPushButton::clicked, this, &MainWindow::close);

  headerLayout->addWidget(btnMin);
  headerLayout->addWidget(btnMax);
  headerLayout->addWidget(btnClose);

  // --- BODY CONTAINER (Sidebar + Content) ---
  QWidget *bodyWidget = new QWidget();
  bodyWidget->setStyleSheet("background: transparent;");
  QHBoxLayout *bodyLayout = new QHBoxLayout(bodyWidget);
  bodyLayout->setContentsMargins(0, 0, 0, 0);
  bodyLayout->setSpacing(0);

  mainLayout->addWidget(bodyWidget);

  // Sidebar (Reuse existing setup)
  setupSidebar();
  m_sidebarWidget->hide(); // Hide by default (Login screen first)
  bodyLayout->addWidget(m_sidebarWidget);

  // Main Content Stack
  m_mainWidget =
      new QWidget(); // Rename effectively acts as container for stack
  // Actually usually m_mainWidget was the right side container.
  // Let's keep m_mainWidget as the standard right side container
  m_mainWidget->setStyleSheet("background: transparent;");
  QVBoxLayout *rightLayout = new QVBoxLayout(m_mainWidget);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(0);

  // We don't add topHeader to rightLayout anymore.

  // Initialize Pages Stack
  m_stackedWidget = new QStackedWidget();
  rightLayout->addWidget(m_stackedWidget);

  bodyLayout->addWidget(m_mainWidget);

  // Final Polish
  setupHomeUI();
  setupAnalyticsUI();
  setupDataProcessingUI();
  setupCreativeToolsUI();
  // Admin UI might be setup inside one of these or we need to add it if
  // separate

  updateNavStyles(m_homeBtn);

  setupLoginUI();
  showSplash();
}

void MainWindow::showSplash() {
  m_splashWidget = new QWidget();
  m_splashWidget->setStyleSheet("background: #0f172a;");

  QVBoxLayout *layout = new QVBoxLayout(m_splashWidget);
  layout->setAlignment(Qt::AlignCenter);

  QLabel *logo = new QLabel();
  QPixmap pix(Utils::resourcePath("resources/knight.png"));
  if (!pix.isNull()) {
    logo->setPixmap(
        pix.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
  logo->setAlignment(Qt::AlignCenter);
  layout->addWidget(logo);

  QLabel *title = new QLabel("GOLEVENTS PRO");
  title->setStyleSheet(
      "color: white; font-size: 24px; font-weight: bold; margin-top: 20px;");
  title->setAlignment(Qt::AlignCenter);
  layout->addWidget(title);

  m_loadBar = new QProgressBar();
  m_loadBar->setFixedWidth(300);
  m_loadBar->setStyleSheet(
      "QProgressBar { background: #334155; border-radius: 4px; text-align: "
      "center; color: transparent; } "
      "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, "
      "y2:0, stop:0 #6366f1, stop:1 #a855f7); border-radius: 4px; }");
  layout->addWidget(m_loadBar);

  m_splashMsg = new QLabel("Initializing...");
  m_splashMsg->setStyleSheet(
      "color: #94a3b8; font-size: 12px; margin-top: 10px;");
  m_splashMsg->setAlignment(Qt::AlignCenter);
  layout->addWidget(m_splashMsg);

  m_stackedWidget->addWidget(m_splashWidget);
  m_stackedWidget->setCurrentWidget(m_splashWidget);

  // Simulate loading
  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this, timer]() {
    static int progress = 0;
    progress += 5;
    // Assuming animateLoading is a function that updates m_loadBar and
    // m_splashMsg based on progress. If not, this line might need adjustment or
    // definition. For now, we'll just update the progress bar directly.
    m_loadBar->setValue(progress);
    if (progress >= 100) {
      timer->stop();
      timer->deleteLater();
      m_stackedWidget->setCurrentWidget(m_loginWidget);
    }
  });
  timer->start(50);
}

void MainWindow::setupLoginUI() {
  m_loginWidget = new QWidget();
  // Using a gradient background for login to make it stand out
  m_loginWidget->setStyleSheet(
      "background: qradialgradient(cx:0.5, cy:0.5, radius: 1, fx:0.5, fy:0.5, "
      "stop:0 #1e293b, stop:1 #0f172a);");

  QVBoxLayout *mainLayout = new QVBoxLayout(m_loginWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setAlignment(Qt::AlignCenter);

  // Container
  QWidget *container = new QWidget();
  container->setObjectName("loginContainer");
  container->setFixedSize(420, 500); // Slightly taller
  container->setStyleSheet(
      "QWidget#loginContainer { background: rgba(30, 41, 59, 0.7); border: 1px "
      "solid rgba(255,255,255,0.1); border-radius: 16px; }"); // Explicit
                                                              // styling

  QVBoxLayout *vbox = new QVBoxLayout(container);
  vbox->setSpacing(20);
  vbox->setContentsMargins(40, 40, 40, 40);

  // Logo
  QLabel *iconLabel = new QLabel();
  QPixmap logo(Utils::resourcePath("resources/knight.png"));
  if (!logo.isNull()) {
    iconLabel->setPixmap(
        logo.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("background: transparent; border: none;");
    vbox->addWidget(iconLabel);
  }

  // Title
  QLabel *title = new QLabel("GOLEVENTS PRO");
  title->setObjectName("loginTitle");
  title->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
  title->setAlignment(Qt::AlignCenter);
  vbox->addWidget(title);

  // Subtitle
  QLabel *sub = new QLabel("Global Analytics System");
  sub->setStyleSheet(
      "color: #94A3B8; font-size: 12px; letter-spacing: 1px; font-weight: 500; "
      "background: transparent; border: none;");
  sub->setAlignment(Qt::AlignCenter);
  vbox->addWidget(sub);

  vbox->addSpacing(15);

  // Key Input
  m_keyInput = new QLineEdit();
  m_keyInput->setPlaceholderText("LICENSE KEY");
  m_keyInput->setAlignment(Qt::AlignCenter);
  m_keyInput->setFixedHeight(48);
  // Add some input styling
  m_keyInput->setStyleSheet("QLineEdit { background: rgba(0,0,0,0.3); border: "
                            "1px solid rgba(255,255,255,0.1); border-radius: "
                            "8px; color: white; font-size: 14px; } "
                            "QLineEdit:focus { border: 1px solid #6366f1; }");
  connect(m_keyInput, &QLineEdit::returnPressed, this,
          &MainWindow::processLogin);
  vbox->addWidget(m_keyInput);

  // Login Button - High Visibility (User: "byn lbutton")
  QPushButton *loginBtn = new QPushButton("AUTHENTICATE");
  loginBtn->setObjectName("actionButton");
  loginBtn->setCursor(Qt::PointingHandCursor);
  loginBtn->setFixedHeight(48);
  // Strong gradient style
  loginBtn->setStyleSheet(
      "QPushButton { "
      "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4f46e5, "
      "stop:1 #7c3aed); "
      "color: white; border: none; border-radius: 8px; font-weight: bold; "
      "font-size: 14px; letter-spacing: 1px;"
      "} "
      "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
      "stop:0 #4338ca, stop:1 #6d28d9); } "
      "QPushButton:pressed { background: #312e81; }");

  connect(loginBtn, &QPushButton::clicked, this, &MainWindow::processLogin);
  vbox->addWidget(loginBtn);

  // Status
  m_msgLabel = new QLabel("");
  m_msgLabel->setAlignment(Qt::AlignCenter);
  m_msgLabel->setStyleSheet("color: #F43F5E; font-size: 11px; font-weight: "
                            "bold; background: transparent; padding-top: 5px;");
  vbox->addWidget(m_msgLabel);

  vbox->addStretch();

  QLabel *footer = new QLabel(QString("Build v%1").arg(Utils::VERSION));
  footer->setStyleSheet(
      "color: #475569; font-size: 10px; background: transparent;");
  footer->setAlignment(Qt::AlignCenter);
  vbox->addWidget(footer);

  mainLayout->addWidget(container);
  m_stackedWidget->addWidget(m_loginWidget);
}

void MainWindow::processLogin() {
  QString key = m_keyInput->text().trimmed();
  if (key.isEmpty()) {
    m_msgLabel->setText("Please enter a license key");
    return;
  }

  m_msgLabel->setText("Authenticating...");
  m_msgLabel->setStyleSheet("color: " + Utils::ACCENT_COLOR +
                            "; font-size: 12px;");

  m_authManager->authenticate(key);
}

void MainWindow::onAuthSuccess() {
  Utils::logToFile("Entering onAuthSuccess");
  m_currentUserName = m_authManager->getUserName();
  m_currentRole = m_authManager->getRole();
  m_currentCredits = m_authManager->getCredits();

  // Update Profile UI with fresh data
  if (m_nameLabel)
    m_nameLabel->setText(m_currentUserName.toUpper());
  if (m_roleLabel)
    m_roleLabel->setText(m_currentRole.toUpper());

  // Show Sidebar NOW (was hidden on login)
  m_sidebarWidget->show();

  // Expand to full dashboard size
  resize(1280, 800);
  const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
  move(screen.center() - rect().center());

  // Init Dashboard Pages
  setupHomeUI();
  setupAnalyticsUI();
  setupDataProcessingUI();
  setupCreativeToolsUI();

  // Clear splash/login if needed, or just append
  // Stack order so far: 0=Login, 1=Splash
  // We will append: 2=Home, 3=Analytics, 4=Data, 5=Creative, 6=Admin

  m_stackedWidget->addWidget(m_homeWidget);
  m_stackedWidget->addWidget(m_analyticsWidget);
  m_stackedWidget->addWidget(m_dataWidget);
  m_stackedWidget->addWidget(m_creativeWidget);

  // Navigation Logic
  // Indices mapping:
  // Home -> 2
  // Analytics -> 3
  // Data -> 4
  // Creative -> 5

  // Navigation is already handled by signals connected in setupSidebar()
  // linking to showHome(), showAnalytics() etc.
  // We do NOT need to re-connect them here.

  // Admin Panel
  QString roleLower = m_currentRole.toLower();
  if (roleLower.contains("admin") || roleLower.contains("dev")) {
    m_adminBtn->show();
    if (!m_adminWidget) {
      m_adminWidget = new AdminPanel(m_authManager);
    }
    m_stackedWidget->addWidget(m_adminWidget);

    connect(m_adminBtn, &QPushButton::clicked, [this]() {
      m_stackedWidget->setCurrentWidget(m_adminWidget);
      updateNavStyles(m_adminBtn);
    });
  } else {
    m_adminBtn->hide();
  }

  // Show Sidebar and go to Home
  m_sidebarWidget->show();
  m_stackedWidget->setCurrentWidget(m_homeWidget);
  updateNavStyles(m_homeBtn);
  // Cleanup Login/Splash resources
  if (m_splashWidget) {
    m_stackedWidget->removeWidget(m_splashWidget);
    m_splashWidget->deleteLater();
    m_splashWidget = nullptr;
  }
  if (m_loginWidget) {
    m_stackedWidget->removeWidget(m_loginWidget);
    m_loginWidget->deleteLater();
    m_loginWidget = nullptr;
  }
}

void MainWindow::onAuthFailed(const QString &reason) {
  QString cleanReason = reason;
  // If reason is "License key is empty", keep it simple.
  // For critical errors (Banned, Inactive, Expired, HWID), append contact info.
  bool critical = reason.contains("BANNED") || reason.contains("INACTIVE") ||
                  reason.contains("EXPIRED") || reason.contains("HWID") ||
                  reason.contains("Security");

  if (critical) {
    cleanReason += "\n\nCONTACT OMAR FOR SUPPORT.";
  }

  m_msgLabel->setText(cleanReason);
  m_msgLabel->setStyleSheet("color: #ef4444; font-size: 13px; font-weight: "
                            "bold; background: transparent;");

  // Show popup as requested (Cosmic Dialog)
  CosmicDialog::show("Access Denied", cleanReason, CosmicDialog::Error, this);

  // Log current HWID to help user debug device registration
  if (m_logBox) {
    m_logBox->append("<font color='#94a3b8'>[SYSTEM] Current HWID: " +
                     Utils::getHWID() + "</font>");
  }
}

void MainWindow::setupSidebar() {
  m_sidebarWidget = new QWidget();
  m_sidebarWidget->setFixedWidth(280); // Match screenshot width
  m_sidebarWidget->setObjectName("sidebar");
  m_sidebarWidget->setStyleSheet(
      "background-color: #0b0f19; border-right: 1px solid #1e293b;");

  QVBoxLayout *layout = new QVBoxLayout(m_sidebarWidget);
  layout->setContentsMargins(0, 40, 0, 0);
  layout->setSpacing(0);
  layout->setAlignment(Qt::AlignTop);

  // --- HEADER: GOLEVENTS ANALYTICS ENGINE ---
  m_sidebarHeader = new QWidget();
  QVBoxLayout *headerLayout = new QVBoxLayout(m_sidebarHeader);
  headerLayout->setContentsMargins(30, 0, 30, 40);
  headerLayout->setSpacing(5);

  m_sidebarTitle = new QLabel("GOLEVENTS");
  m_sidebarTitle->setStyleSheet(
      "color: white; font-size: 24px; font-weight: 900; letter-spacing: 1px; "
      "background: transparent; border: none;");

  m_sidebarSubtitle = new QLabel("ANALYTICS ENGINE");
  m_sidebarSubtitle->setStyleSheet(
      "color: #3b82f6; font-size: 10px; font-weight: 700; "
      "letter-spacing: 1px; background: transparent; border: none;");

  headerLayout->addWidget(m_sidebarTitle);
  headerLayout->addWidget(m_sidebarSubtitle);
  layout->addWidget(m_sidebarHeader);

  // --- NAVIGATION ---
  auto createNavBtn = [this](const QString &text,
                             const QString &icon) -> QPushButton * {
    QPushButton *btn = new QPushButton("  " + icon + "    " + text);
    btn->setProperty("fullText", "  " + icon + "    " + text);
    btn->setProperty("iconOnly", icon);
    btn->setFixedHeight(55);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    btn->setAutoExclusive(true);
    btn->setStyleSheet(
        "QPushButton { "
        "   background: transparent; color: #94a3b8; border: none; "
        "   text-align: left; padding-left: 30px; font-size: 14px; "
        "font-weight: 600;"
        "} "
        "QPushButton:hover { background-color: rgba(255,255,255,0.03); color: "
        "white; } "
        "QPushButton:checked { "
        "   background-color: rgba(15, 23, 42, 0.5); "
        "   color: white; "
        "   border-left: 3px solid #3b82f6; "
        "}");
    return btn;
  };

  m_homeBtn = createNavBtn("Dashboard", "🏠");
  connect(m_homeBtn, &QPushButton::clicked, this, &MainWindow::showHome);
  layout->addWidget(m_homeBtn);

  m_analyticsBtn = createNavBtn("Analytics", "📊");
  connect(m_analyticsBtn, &QPushButton::clicked, this,
          &MainWindow::showAnalytics);
  layout->addWidget(m_analyticsBtn);

  // Match Screenshot: "Data Engine" text
  m_dataBtn = createNavBtn("Data Engine", "⚙️");
  connect(m_dataBtn, &QPushButton::clicked, this,
          &MainWindow::showDataProcessing);
  layout->addWidget(m_dataBtn);

  m_creativeBtn = createNavBtn("Visuals", "🎨");
  connect(m_creativeBtn, &QPushButton::clicked, this,
          &MainWindow::showCreativeTools);
  layout->addWidget(m_creativeBtn);

  /* Settings button removed from sidebar as requested ("hadi t7yd mn hna")
  m_settingsBtn = createNavBtn("Settings", "⚙️");
  connect(m_settingsBtn, &QPushButton::clicked, this,
          &MainWindow::showSettings);
  layout->addWidget(m_settingsBtn);
  */

  m_adminBtn = createNavBtn("Admin Panel", "🔒");
  connect(m_adminBtn, &QPushButton::clicked, this, &MainWindow::showAdmin);
  layout->addWidget(m_adminBtn);

  QString roleLower = m_currentRole.toLower();
  if (!roleLower.contains("admin") && !roleLower.contains("dev")) {
    m_adminBtn->hide();
  }

  layout->addStretch();

  // --- BOTTOM: PROFILE PANEL ---
  QFrame *profilePanel = new QFrame();
  profilePanel->setObjectName("profileCard");
  profilePanel->setFixedHeight(120);
  profilePanel->setStyleSheet("QFrame#profileCard { background-color: "
                              "transparent; "
                              "border-top: 1px solid rgba(255, 255, 255, 0.1); "
                              "}"); // Transparent background to show
                                    // AnimatedBackground

  QVBoxLayout *pLayout = new QVBoxLayout(profilePanel);
  pLayout->setContentsMargins(30, 20, 30, 20);
  pLayout->setSpacing(5);

  // Row 1: Avatar + Name
  QHBoxLayout *infoRow = new QHBoxLayout();

  m_avatarBtn = new QPushButton("👤");
  m_avatarBtn->setFixedSize(44, 44);
  m_avatarBtn->setCursor(Qt::PointingHandCursor);
  m_avatarBtn->setStyleSheet(
      "QPushButton { background-color: rgba(255,255,255,0.05); border: 1px "
      "solid rgba(255,255,255,0.1); border-radius: 22px; font-size: 20px; "
      "color: white; } "
      "QPushButton:hover { background-color: rgba(255,255,255,0.1); "
      "border-color: rgba(30, 255, 180, 0.4); }");
  connect(m_avatarBtn, &QPushButton::clicked, this,
          &MainWindow::onAvatarClicked);

  m_profileDetails = new QWidget();
  QVBoxLayout *nameBox = new QVBoxLayout(m_profileDetails);
  nameBox->setContentsMargins(0, 0, 0, 0);
  nameBox->setSpacing(2);
  m_nameLabel = new QLabel(m_currentUserName.toUpper());
  m_nameLabel->setStyleSheet(
      "color: white; font-weight: bold; font-size: 13px; "
      "background: transparent; border: none;");
  m_roleLabel = new QLabel(m_currentRole.toUpper());
  m_roleLabel->setStyleSheet(
      "color: #fbbf24; font-weight: bold; font-size: 10px; background: "
      "transparent; border: none;"); // Amber Role

  nameBox->addWidget(m_nameLabel);
  nameBox->addWidget(m_roleLabel);

  // Row 2: Lifetime Status
  QLabel *lifetime = new QLabel("📅  Lifetime");
  lifetime->setStyleSheet(
      "color: #fbbf24; font-size: 11px; margin-top: 5px; font-weight: 600; "
      "background: transparent; border: none;");
  nameBox->addWidget(lifetime);

  infoRow->addWidget(m_avatarBtn);
  infoRow->addSpacing(10);
  infoRow->addWidget(m_profileDetails);
  infoRow->addStretch();

  pLayout->addLayout(infoRow);

  layout->addWidget(profilePanel);
}

void MainWindow::updateNavStyles(QPushButton *activeBtn) {
  m_homeBtn->setChecked(activeBtn == m_homeBtn);
  m_analyticsBtn->setChecked(activeBtn == m_analyticsBtn);
  m_dataBtn->setChecked(activeBtn == m_dataBtn);
  m_creativeBtn->setChecked(activeBtn == m_creativeBtn);
  if (m_adminBtn)
    m_adminBtn->setChecked(activeBtn == m_adminBtn);
}

void MainWindow::setupHomeUI() {
  m_homeWidget = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(m_homeWidget);
  layout->setContentsMargins(50, 40, 50, 40);
  layout->setSpacing(30);

  // --- HEADER: DASHBOARD OVERVIEW ---
  QLabel *header = new QLabel("DASHBOARD OVERVIEW");
  header->setStyleSheet("color: #5b7cfd; font-size: 28px; font-weight: 900; "
                        "letter-spacing: 1px;");
  layout->addWidget(header);

  QWidget *cardsContainer = new QWidget();
  cardsContainer->setStyleSheet("background: transparent;");
  QHBoxLayout *cardsLayout = new QHBoxLayout(cardsContainer);
  cardsLayout->setSpacing(20);
  cardsLayout->setContentsMargins(0, 0, 0, 0);

  auto createDetailCard = [](const QString &icon, const QString &label,
                             const QString &value,
                             const QString &valColor) -> QFrame * {
    QFrame *card = new QFrame();
    card->setStyleSheet("background-color: rgba(255, 255, 255, 0.03); "
                        "border: 1px solid rgba(255, 255, 255, 0.08); "
                        "border-radius: 16px;");
    card->setFixedHeight(70);

    QVBoxLayout *cl = new QVBoxLayout(card);
    cl->setContentsMargins(12, 10, 12, 10);
    cl->setSpacing(4);
    cl->setAlignment(Qt::AlignLeft);

    QLabel *lblObj = new QLabel(icon + "   " + label);
    lblObj->setStyleSheet(
        "color: #64748b; font-weight: bold; font-size: 9px; "
        "letter-spacing: 0.5px; background: transparent; border: none;");

    QLabel *valObj = new QLabel(value);
    valObj->setStyleSheet(
        QString("color: %1; font-weight: bold; font-size: 11px; background: "
                "transparent; border: none;")
            .arg(valColor));

    cl->addWidget(lblObj);
    cl->addWidget(valObj);
    return card;
  };

  // Card 1: Member Name
  cardsLayout->addWidget(createDetailCard(
      "👤", "MEMBER NAME", m_currentUserName.toUpper(), "white"));

  // Card 2: Account Type
  cardsLayout->addWidget(createDetailCard(
      "👑", "ACCOUNT TYPE", m_currentRole.toUpper(), "#fbbf24")); // Amber

  cardsLayout->addWidget(createDetailCard("⏳", "SUBSCRIPTION ENDS", "Lifetime",
                                          "#fbbf24")); // Amber

  layout->addWidget(cardsContainer);

  // --- MIDDLE: SYSTEM PERSPECTIVE HELPER ---
  QFrame *perspectivePanel = new QFrame();
  perspectivePanel->setStyleSheet(
      "background-color: rgba(255, 255, 255, 0.03); "
      "border: 1px solid rgba(255, 255, 255, 0.08); "
      "border-radius: 16px;");

  QVBoxLayout *ppLayout = new QVBoxLayout(perspectivePanel);
  ppLayout->setContentsMargins(30, 30, 30, 30);
  ppLayout->setSpacing(15);

  // Header with Pin
  QLabel *ppHeader = new QLabel("📌 SYSTEM PERSPECTIVE");
  ppHeader->setStyleSheet(
      "color: white; font-weight: 800; font-size: 16px; letter-spacing: 0.5px; "
      "border-bottom: 2px solid rgba(255,255,255,0.1); padding-bottom: 10px; "
      "background: transparent; border: none;");
  ppLayout->addWidget(ppHeader);

  // List
  QStringList points = {"System Identity: Super Admin secured.",
                        "Access Level: ADMIN features unlocked.",
                        "Security Status: 256-bit encryption active.",
                        "Network Latency: Sub-10ms response time."};

  for (const QString &pt : points) {
    QLabel *ptLbl = new QLabel("• " + pt);
    ptLbl->setStyleSheet(
        "color: #94a3b8; font-size: 13px; font-weight: 500; margin-left: 5px; "
        "background: transparent; border: none;");
    ppLayout->addWidget(ptLbl);
  }

  layout->addWidget(perspectivePanel);

  // --- BOTTOM: LOG / TERMINAL (Unchanged but styled to match) ---
  m_logBox = new QTextEdit();
  m_logBox->setReadOnly(true);
  m_logBox->setFixedHeight(120);
  m_logBox->setStyleSheet(
      "QTextEdit { background: rgba(0, 0, 0, 0.15); color: #10b981; "
      "border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 8px; "
      "font-family: 'Consolas', 'Courier New'; font-size: 12px; padding: 15px; "
      "}");

  // Mockup Initial Logs
  QTime now = QTime::currentTime();
  m_logBox->append("[" + now.toString("HH:mm:ss") +
                   "] SYSTEM: Initializing services...");
  m_logBox->append("[" + now.toString("HH:mm:ss") + "] STATUS: System Ready.");

  layout->addWidget(m_logBox);
  layout->addStretch();
}

QWidget *MainWindow::createStatCard(const QString &title, const QString &value,
                                    const QString &color, const QString &icon) {
  QFrame *card = new QFrame();
  card->setFixedHeight(70);
  card->setStyleSheet(
      QString("QFrame { background: rgba(255, 255, 255, 0.03); "
              "border: 1px solid rgba(255, 255, 255, 0.08); "
              "border-radius: 16px; }"
              "QFrame:hover { background: rgba(255, 255, 255, 0.06); "
              "border: 1px solid rgba(255, 255, 255, 0.15); }")
          .arg(Utils::ACCENT_COLOR));

  // Add Drop Shadow
  QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
  effect->setBlurRadius(20);
  effect->setOffset(0, 4);
  effect->setColor(QColor(0, 0, 0, 100));
  card->setGraphicsEffect(effect);

  QVBoxLayout *layout = new QVBoxLayout(card);
  layout->setContentsMargins(12, 10, 12, 10);
  layout->setSpacing(4);

  QHBoxLayout *top = new QHBoxLayout();
  QLabel *iconLabel = new QLabel(icon);
  iconLabel->setAttribute(Qt::WA_TranslucentBackground);
  iconLabel->setStyleSheet(
      "font-size: 20px; background: transparent; border: none;");
  top->addWidget(iconLabel);
  top->addStretch();

  QLabel *titleLabel = new QLabel(title);
  titleLabel->setAttribute(Qt::WA_TranslucentBackground);
  titleLabel->setStyleSheet(
      "color: #6b7280; font-size: 9px; font-weight: 700; text-transform: "
      "uppercase; letter-spacing: 1px; background: transparent; border: "
      "none;");
  top->addWidget(titleLabel);

  layout->addLayout(top);
  layout->addSpacing(4);

  QLabel *valueLabel = new QLabel(value);
  valueLabel->setAttribute(Qt::WA_TranslucentBackground);
  valueLabel->setStyleSheet(
      QString("color: %1; font-size: 12px; font-weight: 800; background: "
              "transparent; border: none;")
          .arg(color));
  layout->addWidget(valueLabel);

  layout->addStretch();
  return card;
}

void MainWindow::setupAnalyticsUI() {
  m_analyticsWidget = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(m_analyticsWidget);

  QLabel *title = new QLabel("ANALYTICS & VERIFICATION");
  title->setStyleSheet("color: white; font-size: 28px; font-weight: 900; "
                       "letter-spacing: 1px; background: transparent;");
  layout->addWidget(title);
  layout->addSpacing(10);

  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setStyleSheet(
      "QScrollArea { border: none; background: transparent; }");

  QWidget *container = new QWidget();
  FlowLayout *flowLayout = new FlowLayout(container, 0, 15, 15);

  // Listing Checker
  flowLayout->addWidget(createToolCard(
      "Listing Checker", "Verify listings status.", "Check_listing", true));

  // Order Verifier
  flowLayout->addWidget(createToolCard(
      "Order Verifier", "Ensure orders accuracy.", "verify_orders", true));

  // Price Check
  flowLayout->addWidget(createToolCard("Price Check", "Monitor market prices.",
                                       "check_price", true));

  // Stock Calc
  flowLayout->addWidget(createToolCard(
      "Stock Calc", "Inventory stock calculator.", "Calcul_stock", false));

  scrollArea->setWidget(container);
  layout->addWidget(scrollArea);
}

void MainWindow::setupDataProcessingUI() {
  m_dataWidget = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(m_dataWidget);

  QLabel *title = new QLabel("DATA PROCESSING");
  title->setStyleSheet("color: white; font-size: 28px; font-weight: 900; "
                       "letter-spacing: 1px; background: transparent;");
  layout->addWidget(title);
  layout->addSpacing(10);

  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setStyleSheet(
      "QScrollArea { border: none; background: transparent; }");

  QWidget *container = new QWidget();
  FlowLayout *flowLayout = new FlowLayout(container, 0, 15, 15);

  // PDF to TXT
  flowLayout->addWidget(createToolCard(
      "PDF to TXT", "Extract PDF data to text.", "pdfs_to_txt", false));

  // Splitter+Renamer
  flowLayout->addWidget(createToolCard(
      "Splitter+Renamer", "PDF split and rename.", "splitter_renamer", false));

  // Stock Report
  flowLayout->addWidget(createToolCard("Stock Report", "Net stock vs Orders.",
                                       "stock_report", true));

  // Daily Report (Use Calendar icon, not the specific one yet unless provided)
  flowLayout->addWidget(createToolCard("Daily Report", "Daily automated tasks.",
                                       "daily_report", false));

  // Renamer + FV
  flowLayout->addWidget(createToolCard("Renamer + FV", "Rename files with FV.",
                                       "renamer_fv", false));

  scrollArea->setWidget(container);
  layout->addWidget(scrollArea);
}

void MainWindow::setupCreativeToolsUI() {
  m_creativeWidget = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(m_creativeWidget);

  QLabel *title = new QLabel("CREATIVE TOOLS");
  title->setStyleSheet("color: white; font-size: 28px; font-weight: 900; "
                       "letter-spacing: 1px; background: transparent;");
  layout->addWidget(title);
  layout->addSpacing(10);

  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setStyleSheet(
      "QScrollArea { border: none; background: transparent; }");

  QWidget *container = new QWidget();
  FlowLayout *flowLayout = new FlowLayout(container, 0, 15, 15);

  // Seat Expander
  flowLayout->addWidget(createToolCard(
      "Seat Expander", "Seating map generator.", "expander_seats", false));

  // QR Generator
  flowLayout->addWidget(
      createToolCard("QR Generator", "Batch QR Codes.", "qr_generator", false));

  // Placeholder
  flowLayout->addWidget(createToolCard(
      "Placeholder", "Temporary PDF generator.", "placeholder", false));

  scrollArea->setWidget(container);
  layout->addWidget(scrollArea);
}

ToolCard *MainWindow::createToolCard(const QString &title, const QString &desc,
                                     const QString &scriptName, bool isPro) {
  // Determine icon based on script name or title
  QString icon = "🛠️"; // Default
  if (scriptName == "Check_listing")
    icon = "📋";
  else if (scriptName == "verify_orders")
    icon = "✅";
  else if (scriptName == "check_price")
    icon = "💰";
  else if (scriptName == "Calcul_stock")
    icon = "🧮";
  else if (scriptName == "pdfs_to_txt")
    icon = "📄";
  else if (scriptName == "splitter_renamer")
    icon = "✂️";
  else if (scriptName == "stock_report")
    icon = "📉";
  else if (scriptName == "daily_report")
    icon = "📅";
  else if (scriptName == "renamer_fv")
    icon = "🏷️";
  else if (scriptName == "expander_seats")
    icon = "🏟️";
  else if (scriptName == "qr_generator")
    icon = "🔳";
  else if (scriptName == "placeholder")
    icon = "🧩";
  else if (scriptName == "ai_video")
    icon = "🎥";
  else if (scriptName == "image_enhancer")
    icon = "🪄";
  else if (scriptName == "motion_graphics")
    icon = "🎞️";

  bool locked = isPro && !isUserPro();

  // Create ToolCard instance
  // Constructor: ToolCard(title, desc, iconPath, scriptName, isLocked, parent)
  ToolCard *card = new ToolCard(title, desc, icon, scriptName, locked, this);

  // Connect signals to MainWindow slots
  connect(card, &ToolCard::startClicked, this, &MainWindow::launchTool);
  connect(card, &ToolCard::guideClicked, this, &MainWindow::showGuide);

  return card;
}

void MainWindow::launchTool(const QString &toolName) {
  // Security Check: Prevent non-Pro users from accessing Pro tools
  // even if they bypass the UI lock.
  QStringList proTools = {"Check_listing", "verify_orders", "check_price",
                          "stock_report"};

  if (proTools.contains(toolName) && !isUserPro()) {
    CosmicDialog::show("Access Denied",
                       "This tool is restricted to PRO users.\nContact support "
                       "to upgrade your license.",
                       CosmicDialog::Error, this);
    return;
  }

  QDialog *toolDialog = nullptr;

  if (toolName == "Calcul_stock") {
    toolDialog = new CalcStock(this);
  } else if (toolName == "daily_report") {
    toolDialog = new DailyReport(this);
  } else if (toolName == "check_price") {
    toolDialog = new CheckPrice(this);
  } else if (toolName == "expander_seats") {
    toolDialog = new ExpanderSeats(this);
  } else if (toolName == "pdfs_to_txt") {
    toolDialog = new PdfsToTxt(this);
  } else if (toolName == "placeholder") {
    toolDialog = new Placeholder(this);
  } else if (toolName == "qr_generator") {
    toolDialog = new QrGenerator(this);
  } else if (toolName == "Check_listing") {
    toolDialog = new CheckListing(this);
  } else if (toolName == "verify_orders") {
    toolDialog = new VerifyOrders(this);
  } else if (toolName == "renamer_fv") {
    toolDialog = new RenamerFv(this);
  } else if (toolName == "splitter_renamer") {
    toolDialog = new SplitterRenamer(this);
  } else if (toolName == "stock_report") {
    toolDialog = new StockReport(this);
  }

  if (toolDialog) {
    toolDialog->exec();
    delete toolDialog;
  }
}

void MainWindow::showGuide(const QString &toolName,
                           const QString &displayName) {
  GuideDialog dialog(toolName, displayName, this);
  dialog.exec();
}

void MainWindow::applyDarkTheme() {
  setStyleSheet(
      QString("QMainWindow { background: %1; }"
              "QWidget { color: white; font-family: 'Inter', 'Segoe UI', "
              "system-ui; }"
              "QLabel { background: transparent !important; border: none "
              "!important; }"
              "QFrame { background: transparent; border: none; }"
              "QScrollArea { border: none; background: transparent; }"
              "QScrollArea QWidget { background: transparent; }"
              "QScrollBar:vertical { background: transparent; width: 6px; }"
              "QScrollBar::handle:vertical { background: #1f2937; "
              "border-radius: 3px; }"
              "QScrollBar::handle:vertical:hover { background: #374151; }"
              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
              "height: 0px; }"
              "QMessageBox { background-color: #09090b; border: 1px solid "
              "#1f2937; border-radius: 12px; }"
              "QMessageBox QLabel { color: white; font-size: 13px; }"
              "QMessageBox QPushButton { background: %2; color: white; "
              "border-radius: 6px; padding: 6px 14px; font-weight: bold; "
              "border: none; }"
              "QMessageBox QPushButton:hover { opacity: 0.9; }")
          .arg(Utils::BG_COLOR, Utils::ACCENT_COLOR));
}

void MainWindow::showHome() {
  updateNavStyles(m_homeBtn);
  switchToPage(m_homeWidget);
}

void MainWindow::showAnalytics() {
  updateNavStyles(m_analyticsBtn);
  switchToPage(m_analyticsWidget);
}

void MainWindow::showDataProcessing() {
  updateNavStyles(m_dataBtn);
  switchToPage(m_dataWidget);
}

void MainWindow::showCreativeTools() {
  updateNavStyles(m_creativeBtn);
  switchToPage(m_creativeWidget);
}

void MainWindow::showSettings() {
  updateNavStyles(m_settingsBtn);
  // Lazy init if we want, or just set current index if added to stack
  // Searching for SettingsPage in stack...
  SettingsPage *targetPage = nullptr;
  for (int i = 0; i < m_stackedWidget->count(); ++i) {
    if (SettingsPage *p =
            qobject_cast<SettingsPage *>(m_stackedWidget->widget(i))) {
      targetPage = p;
      break;
    }
  }
  // If not found, create and add
  if (!targetPage) {
    targetPage = new SettingsPage();
    m_stackedWidget->addWidget(targetPage);
  }
  switchToPage(targetPage);
}

void MainWindow::showAdmin() {
  if (m_adminBtn->isVisible()) {
    updateNavStyles(m_adminBtn);
    if (m_adminWidget) {
      static_cast<AdminPanel *>(m_adminWidget)->refreshData();
      switchToPage(m_adminWidget);
    }
  }
}

void MainWindow::switchToPage(QWidget *newPage) {
  if (!newPage || m_stackedWidget->currentWidget() == newPage)
    return;

  // 1. Capture current visual state
  QPixmap currentPix = m_stackedWidget->grab();

  // 2. Create overlay logic
  // We create a label on top of the stacked widget to hold the old screenshot
  QLabel *overlay = new QLabel(m_stackedWidget);
  overlay->setPixmap(currentPix);
  overlay->setGeometry(m_stackedWidget->rect());
  overlay->show();
  overlay->raise(); // Ensure it's on top

  // 3. Switch page immediately (hidden behind overlay)
  m_stackedWidget->setCurrentWidget(newPage);

  // 4. Animate overlay opacity (Fade Out)
  QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(overlay);
  overlay->setGraphicsEffect(effect);

  QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
  anim->setDuration(250); // 250ms smooth transition
  anim->setStartValue(1.0);
  anim->setEndValue(0.0);
  anim->setEasingCurve(QEasingCurve::OutQuad);

  // 5. Cleanup
  connect(anim, &QPropertyAnimation::finished,
          [overlay]() { overlay->deleteLater(); });

  anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onUpdateAvailable(const QString &version, const QString &url) {
  QMessageBox msgBox;
  msgBox.setWindowTitle("⚠️ CRITICAL UPDATE REQUIRED");
  msgBox.setText(
      QString("NEW VERSION AVAILABLE: %1\n\n"
              "This is a MANDATORY update.\n"
              "You cannot use the application without this update.\n\n"
              "Please click 'Update Now' to get the latest version.")
          .arg(version));
  msgBox.setIcon(QMessageBox::Critical);

  // Only allow Update or Exit. No cancel.
  QPushButton *updateBtn =
      msgBox.addButton("⬇️ CHECK UPDATE", QMessageBox::YesRole);
  QPushButton *exitBtn = msgBox.addButton("EXIT APP", QMessageBox::NoRole);

  msgBox.setStyleSheet(
      QString("QMessageBox { background: #0f172a; border: 2px solid #ef4444; } "
              "QLabel { color: white; font-size: 14px; font-weight: bold; } "
              "QPushButton { background: %1; color: white; border-radius: 5px; "
              "padding: 8px 16px; font-weight: bold; } "
              "QPushButton:hover { background: #3b82f6; }")
          .arg(Utils::ACCENT_COLOR));

  msgBox.exec();

  if (msgBox.clickedButton() == updateBtn) {
    QDesktopServices::openUrl(QUrl(url));
    QApplication::quit();
  } else {
    QApplication::quit();
  }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);

  if (m_mainWidget) {
    bool shouldBeCompact = width() < 700;
    if (shouldBeCompact != m_isSidebarCompact) {
      setSidebarCompact(shouldBeCompact);
    }
  }
}

void MainWindow::setSidebarCompact(bool compact) {
  m_isSidebarCompact = compact;
  if (!m_sidebarWidget)
    return;

  if (compact) {
    m_sidebarWidget->setFixedWidth(80);
    m_sidebarTitle->setText("G");
    m_sidebarTitle->setAlignment(Qt::AlignCenter);
    m_sidebarSubtitle->hide();
    m_sidebarHeader->layout()->setContentsMargins(0, 0, 0, 40);

    // Update Nav Buttons to Icon Only
    QList<QPushButton *> navBtns = {m_homeBtn, m_analyticsBtn, m_dataBtn,
                                    m_creativeBtn, m_adminBtn};

    for (QPushButton *btn : navBtns) {
      if (btn) {
        btn->setText(btn->property("iconOnly").toString());
        btn->setStyleSheet(btn->styleSheet().replace(
            "text-align: left; padding-left: 30px;",
            "text-align: center; padding-left: 0px;"));
      }
    }

    m_profileDetails->hide();
    m_avatarBtn->parentWidget()->layout()->setContentsMargins(0, 20, 0, 20);
    // Center avatar in sidebar footer
    static_cast<QHBoxLayout *>(m_avatarBtn->parentWidget()->layout())
        ->setSpacing(0);
    static_cast<QHBoxLayout *>(m_avatarBtn->parentWidget()->layout())
        ->setAlignment(Qt::AlignCenter);

  } else {
    m_sidebarWidget->setFixedWidth(280);
    m_sidebarTitle->setText("GOLEVENTS");
    m_sidebarTitle->setAlignment(Qt::AlignLeft);
    m_sidebarSubtitle->show();
    m_sidebarHeader->layout()->setContentsMargins(30, 0, 30, 40);

    // Update Nav Buttons to Full Text
    QList<QPushButton *> navBtns = {m_homeBtn, m_analyticsBtn, m_dataBtn,
                                    m_creativeBtn, m_adminBtn};

    for (QPushButton *btn : navBtns) {
      if (btn) {
        btn->setText(btn->property("fullText").toString());
        btn->setStyleSheet(
            btn->styleSheet().replace("text-align: center; padding-left: 0px;",
                                      "text-align: left; padding-left: 30px;"));
      }
    }

    m_profileDetails->show();
    m_avatarBtn->parentWidget()->layout()->setContentsMargins(30, 20, 30, 20);
    static_cast<QHBoxLayout *>(m_avatarBtn->parentWidget()->layout())
        ->setSpacing(10);
    static_cast<QHBoxLayout *>(m_avatarBtn->parentWidget()->layout())
        ->setAlignment(Qt::AlignLeft);
  }
}

void MainWindow::onAvatarClicked() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Select Avatar", "", "Image Files (*.png *.jpg *.jpeg *.bmp)");
  if (!fileName.isEmpty()) {
    QPixmap pix(fileName);
    if (!pix.isNull()) {
      // Create rounded avatar
      QPixmap rounded(m_avatarBtn->size());
      rounded.fill(Qt::transparent);

      QPainter painter(&rounded);
      painter.setRenderHint(QPainter::Antialiasing);
      QPainterPath path;
      path.addEllipse(rounded.rect());
      painter.setClipPath(path);
      painter.drawPixmap(rounded.rect(),
                         pix.scaled(m_avatarBtn->size(),
                                    Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation));

      m_avatarBtn->setText(""); // Clear emoji
      m_avatarBtn->setIcon(QIcon(rounded));
      m_avatarBtn->setIconSize(m_avatarBtn->size());
    }
  }
}

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message,
                             qintptr *result) {
#ifdef Q_OS_WIN
  MSG *msg = static_cast<MSG *>(message);
  if (msg->message == WM_NCHITTEST) {
    // Get mouse coordinates (global)
    int xPos = GET_X_LPARAM(msg->lParam);
    int yPos = GET_Y_LPARAM(msg->lParam);
    QPoint pos = mapFromGlobal(QPoint(xPos, yPos));

    const int borderWidth = 8;   // Resize border thickness
    const int headerHeight = 40; // Draggable header height (Matched to UI)

    int w = width();
    int h = height();

    bool left = pos.x() < borderWidth;
    bool right = pos.x() >= w - borderWidth;
    bool top = pos.y() < borderWidth;
    bool bottom = pos.y() >= h - borderWidth;

    if (left && top) {
      *result = HTTOPLEFT;
      return true;
    }
    if (right && top) {
      *result = HTTOPRIGHT;
      return true;
    }
    if (left && bottom) {
      *result = HTBOTTOMLEFT;
      return true;
    }
    if (right && bottom) {
      *result = HTBOTTOMRIGHT;
      return true;
    }
    if (left) {
      *result = HTLEFT;
      return true;
    }
    if (right) {
      *result = HTRIGHT;
      return true;
    }
    if (top) {
      *result = HTTOP;
      return true;
    }
    if (bottom) {
      *result = HTBOTTOM;
      return true;
    }

    // Header dragging (Title Bar) behavior
    // We must exclude buttons from this area so they remain clickable.
    // But nativeEvent happens before Qt widget handling usually.
    // However, returning HTCAPTION usually allows child widgets to still
    // receive clicks if they are NOT transparent to mouse. Actually, returning
    // HTCAPTION swallows clicks unless handled carefully. Standard approach:
    // Check if we are over a child widget that accepts input. For simplicity:
    // If we are in the header area, we return HTCAPTION. The standard Windows
    // behavior: HTCAPTION still allows child validation if we don't block it?
    // Actually, if we return HTCAPTION, the window manager handles it as a
    // drag. Simple workaround: Since our buttons (Minimize/Close) are QWidgets,
    // Qt *should* handle them if we don't return HTCAPTION over them. But
    // nativeEvent is very early loop. Let's rely on Qt's handling: if we return
    // false, Qt handles it. So ONLY return HTCAPTION if we are NOT over a
    // button. This is complex to check from nativeEvent nicely. ALTERNATIVE:
    // Use HTCAPTION for the whole header, and buttons might "just work" or we
    // need to exclude their rects.

    if (pos.y() < headerHeight) {
      // Check if over a button?
      QWidget *child = childAt(pos);
      // If child is a button (or our header buttons), let Qt handle it (return
      // false). If child is the main window itself or a non-interactive
      // container, return HTCAPTION.
      if (!child || child == this || child->inherits("QFrame") ||
          child->objectName() == "header") {
        // Check strictly if it's a QPushButton
        if (child && (child->inherits("QPushButton") ||
                      child->inherits("QAbstractButton"))) {
          return false; // Let Qt handle button click
        }
        *result = HTCAPTION;
        return true;
      }
    }
  }
#endif
  return QMainWindow::nativeEvent(eventType, message, result);
}

// Removed manual mouse handlers to avoid conflict
/*
void MainWindow::updateCursorShape(const QPoint &pos) {
  if (isMaximized() || isFullScreen()) {
    setCursor(Qt::ArrowCursor);
    return;
  }

  int border = 8;
  int w = width();
  int h = height();
  int x = pos.x();
  int y = pos.y();

  Qt::Edges edges;
  if (x < border)
    edges |= Qt::LeftEdge;
  else if (x > w - border)
    edges |= Qt::RightEdge;
  if (y < border)
    edges |= Qt::TopEdge;
  else if (y > h - border)
    edges |= Qt::BottomEdge;

  if (edges == (Qt::LeftEdge | Qt::TopEdge) ||
      edges == (Qt::RightEdge | Qt::BottomEdge))
    setCursor(Qt::SizeFDiagCursor);
  else if (edges == (Qt::RightEdge | Qt::TopEdge) ||
           edges == (Qt::LeftEdge | Qt::BottomEdge))
    setCursor(Qt::SizeBDiagCursor);
  else if (edges & (Qt::LeftEdge | Qt::RightEdge))
    setCursor(Qt::SizeHorCursor);
  else if (edges & (Qt::TopEdge | Qt::BottomEdge))
    setCursor(Qt::SizeVerCursor);
  else
    setCursor(Qt::ArrowCursor);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    int border = 8;
    int w = width();
    int h = height();
    QPoint pos = event->position().toPoint();
    int x = pos.x();
    int y = pos.y();

    m_resizeEdge = Qt::Edges();
    if (x < border)
      m_resizeEdge |= Qt::LeftEdge;
    else if (x > w - border)
      m_resizeEdge |= Qt::RightEdge;
    if (y < border)
      m_resizeEdge |= Qt::TopEdge;
    else if (y > h - border)
      m_resizeEdge |= Qt::BottomEdge;

    if (m_resizeEdge != Qt::Edges()) {
      m_isResizing = true;
      m_dragPos = event->globalPosition().toPoint();
      event->accept();
      return;
    }

    // Check if within top header height (50px)
    if (event->position().y() < 50) {
      m_isResizing = false;
      m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
      event->accept();
      return;
    }
  }
  QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    if (m_isResizing) {
      QPoint globalPos = event->globalPosition().toPoint();
      QPoint delta = globalPos - m_dragPos;
      QRect geom = geometry();

      if (m_resizeEdge & Qt::LeftEdge) {
        geom.setLeft(geom.left() + delta.x());
      }
      if (m_resizeEdge & Qt::RightEdge) {
        geom.setRight(geom.right() + delta.x());
      }
      if (m_resizeEdge & Qt::TopEdge) {
        geom.setTop(geom.top() + delta.y());
      }
      if (m_resizeEdge & Qt::BottomEdge) {
        geom.setBottom(geom.bottom() + delta.y());
      }

      // Enforce minimum size
      if (geom.width() >= minimumWidth() && geom.height() >= minimumHeight()) {
        setGeometry(geom);
        m_dragPos = globalPos; // Update for incremental delta
        // For left/top resizing, keeping dragPos fixed to start might be
        // better, but incremental is easier for simple logic. Actually for
        // setLeft(), we need careful delta handling. Let's rely on setGeometry
        // respecting the rect.
      }
      event->accept();
    } else if (event->position().y() < 50) {
      // Dragging
      move(event->globalPosition().toPoint() - m_dragPos);
      event->accept();
    }
  } else {
    updateCursorShape(event->position().toPoint());
  }
  QMainWindow::mouseMoveEvent(event);
}
*/

} // namespace GOL
