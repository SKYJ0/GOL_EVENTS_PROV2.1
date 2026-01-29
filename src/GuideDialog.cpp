
#include "GuideDialog.h"
#include "Utils.h"
#include <QDir>
#include <QFile>
#include <QPixmap>
#include <QTextStream>

namespace GOL {

GuideDialog::GuideDialog(const QString &toolName, const QString &displayName,
                         QWidget *parent)
    : QDialog(parent), m_currentIndex(0) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog |
                 Qt::WindowStaysOnTopHint | Qt::Tool);
  setAttribute(Qt::WA_TranslucentBackground);
  setModal(true); // Enforce input blocking
  setupUI(displayName);
  loadSteps(toolName);
  updateContent();

  // Modern styling for the dialog itself
  // Semi-transparent dark background (App Theme) as requested: "lon bhal app o
  // chfaff chouia"
  setStyleSheet(QString("QDialog { background-color: rgba(11, 15, 25, 0.95); "
                        "border: 1px solid rgba(255, 255, 255, 0.1); "
                        "border-radius: 20px; }"));
}

void GuideDialog::setupUI(const QString &displayName) {
  setMinimumSize(600, 500);
  setWindowTitle("Guide: " + displayName);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0,
                                 0); // Zero margins for edge-to-edge header
  mainLayout->setSpacing(0);

  // --- Header Container (The "Red Box" area from user request) ---
  QWidget *headerContainer = new QWidget();
  headerContainer->setFixedHeight(70);
  // Transparent background to blend with QDialog background
  headerContainer->setStyleSheet("background-color: transparent; "
                                 "border-bottom: none;");

  QHBoxLayout *headerLayout = new QHBoxLayout(headerContainer);
  headerLayout->setContentsMargins(30, 0, 30,
                                   0); // Padding inside the header strip

  QLabel *titleLabel = new QLabel(displayName.toUpper() + " GUIDE");
  titleLabel->setStyleSheet(
      "color: white; font-size: 22px; font-weight: 900; letter-spacing: 1px; "
      "background: transparent; border: none;");

  QPushButton *closeBtn = new QPushButton("✕");
  closeBtn->setFixedSize(32, 32);
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setStyleSheet("QPushButton { "
                          "  color: white; "
                          "  font-size: 16px; font-weight: bold; "
                          "  background-color: rgba(255, 255, 255, 0.25); "
                          "  border: 1px solid rgba(255, 255, 255, 0.4); "
                          "  border-radius: 16px; "
                          "}"
                          "QPushButton:hover { "
                          "  background-color: #ef4444; "
                          "  border-color: #ef4444; "
                          "  color: white; "
                          "}");
  connect(closeBtn, &QPushButton::clicked, this, &GuideDialog::close);

  headerLayout->addWidget(titleLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(closeBtn);

  mainLayout->addWidget(headerContainer);

  // --- Body Container (Image, Desc, Nav) ---
  QWidget *bodyContainer = new QWidget();
  QVBoxLayout *bodyLayout = new QVBoxLayout(bodyContainer);
  bodyLayout->setContentsMargins(30, 30, 30, 30);
  bodyLayout->setSpacing(20);

  // Image Area
  m_imageLabel = new QLabel();
  m_imageLabel->setFixedSize(540, 300);
  m_imageLabel->setAlignment(Qt::AlignCenter);
  m_imageLabel->setStyleSheet(
      "background: #000; border: 1px solid #1f2937; border-radius: 12px;");
  bodyLayout->addWidget(m_imageLabel);

  // Description Area
  m_descLabel = new QLabel();
  m_descLabel->setWordWrap(true);
  m_descLabel->setStyleSheet("color: #94a3b8; font-size: 14px; line-height: "
                             "20px; background: transparent;");
  bodyLayout->addWidget(m_descLabel);

  bodyLayout->addStretch();

  // Navigation Bar
  QHBoxLayout *navLayout = new QHBoxLayout();

  m_prevBtn = new QPushButton("← PREVIOUS");
  m_prevBtn->setFixedSize(120, 40);
  m_prevBtn->setStyleSheet(
      QString("QPushButton { background: #1f2937; color: white; border-radius: "
              "10px; font-weight: bold; border: none; }"
              "QPushButton:hover { background: #374151; }"
              "QPushButton:disabled { color: #4b5563; background: #111827; }"));

  m_stepCounter = new QLabel("STEP 1 / 1");
  m_stepCounter->setAlignment(Qt::AlignCenter);
  m_stepCounter->setStyleSheet(
      "color: #6366f1; font-weight: 800; font-size: "
      "12px; font-family: 'Inter'; background: transparent;");

  m_nextBtn = new QPushButton("NEXT →");
  m_nextBtn->setFixedSize(120, 40);
  m_nextBtn->setStyleSheet(
      QString("QPushButton { background: %1; color: white; border-radius: "
              "10px; font-weight: bold; border: none; }"
              "QPushButton:hover { opacity: 0.9; }"
              "QPushButton:disabled { background: #111827; color: #4b5563; }")
          .arg(Utils::ACCENT_GRADIENT));

  navLayout->addWidget(m_prevBtn);
  navLayout->addStretch();
  navLayout->addWidget(m_stepCounter);
  navLayout->addStretch();
  navLayout->addWidget(m_nextBtn);

  bodyLayout->addLayout(navLayout);

  mainLayout->addWidget(bodyContainer);

  connect(m_nextBtn, &QPushButton::clicked, this, &GuideDialog::nextStep);
  connect(m_prevBtn, &QPushButton::clicked, this, &GuideDialog::prevStep);
}

void GuideDialog::loadSteps(const QString &toolName) {
  QString guidesPath = Utils::resourcePath("assets/guides/" + toolName);
  QDir dir(guidesPath);

  if (!dir.exists()) {
    m_steps.append(
        {"default_guide.png",
         "Detailed guide content coming soon for " + toolName + "."});
    return;
  }

  // Sort files numerically: 1_img, 1_txt, 2_img, 2_txt...
  QStringList filters;
  filters << "*_img.*";
  QStringList imgFiles = dir.entryList(filters, QDir::Files, QDir::Name);

  for (const QString &imgFile : imgFiles) {
    QString stepNum = imgFile.split('_').first();
    QString txtFile = stepNum + "_txt.txt";

    GuideStep step;
    step.imagePath = dir.absoluteFilePath(imgFile);

    QFile file(dir.absoluteFilePath(txtFile));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);
      step.description = in.readAll();
      file.close();
    } else {
      step.description = "No description available for step " + stepNum;
    }

    m_steps.append(step);
  }

  if (m_steps.isEmpty()) {
    m_steps.append(
        {"default_guide.png",
         "Detailed guide content coming soon for " + toolName + "."});
  }
}

void GuideDialog::nextStep() {
  if (m_currentIndex < m_steps.size() - 1) {
    m_currentIndex++;
    updateContent();
  }
}

void GuideDialog::prevStep() {
  if (m_currentIndex > 0) {
    m_currentIndex--;
    updateContent();
  }
}

void GuideDialog::updateContent() {
  if (m_steps.isEmpty())
    return;

  const auto &step = m_steps[m_currentIndex];

  QPixmap pix(step.imagePath);
  if (!pix.isNull()) {
    m_imageLabel->setPixmap(pix.scaled(
        m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  } else {
    m_imageLabel->setText("IMAGE NOT FOUND");
  }

  m_descLabel->setText(step.description);
  m_stepCounter->setText(
      QString("STEP %1 / %2").arg(m_currentIndex + 1).arg(m_steps.size()));

  m_prevBtn->setEnabled(m_currentIndex > 0);
  m_nextBtn->setEnabled(m_currentIndex < m_steps.size() - 1);
}

} // namespace GOL
