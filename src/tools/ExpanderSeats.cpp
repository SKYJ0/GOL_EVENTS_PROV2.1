#include "ExpanderSeats.h"
#include "../SecurityManager.h"
#include "../Utils.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace GOL {

ExpanderSeats::ExpanderSeats(QWidget *parent) : QDialog(parent) {
  // Security Check
  SecurityManager::instance().checkAndAct();

  setWindowTitle("Seat Expander - GOLEVENTS");
  resize(900, 700);
  setStyleSheet(
      QString("background-color: %1; color: white;").arg(Utils::BG_COLOR));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QLabel *title = new QLabel("💺 SEAT RANGE EXPANDER");
  title->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: bold;")
                           .arg(Utils::ACCENT_COLOR));
  mainLayout->addWidget(title);
  mainLayout->addSpacing(20);

  QLabel *instrLabel = new QLabel(
      "Enter seat ranges (e.g., 'From 10S to 25S' or 'Row 5 Seats 1-10'):");
  instrLabel->setStyleSheet("font-size: 14px;");
  mainLayout->addWidget(instrLabel);

  m_inputText = new QTextEdit();
  m_inputText->setPlaceholderText("From 10S to 25S\nRow 5 Seats 1-10\n100-120");
  m_inputText->setStyleSheet("QTextEdit { background-color: #1a1a1a; color: "
                             "white; border: 1px solid #333; "
                             "border-radius: 8px; padding: 10px; font-family: "
                             "Consolas; font-size: 13px; }");
  mainLayout->addWidget(m_inputText, 1);

  QHBoxLayout *btnLayout = new QHBoxLayout();

  QPushButton *expandBtn = new QPushButton("🚀 EXPAND");
  expandBtn->setStyleSheet(
      QString("QPushButton { background-color: %1; color: white; border: none; "
              "padding: 12px 30px; border-radius: 8px; font-size: 14px; "
              "font-weight: bold; }"
              "QPushButton:hover { background-color: #2a7ab8; }")
          .arg(Utils::ACCENT_COLOR));
  connect(expandBtn, &QPushButton::clicked, this, &ExpanderSeats::expandSeats);
  btnLayout->addWidget(expandBtn);

  QPushButton *clearBtn = new QPushButton("🗑️ CLEAR");
  clearBtn->setStyleSheet(
      "QPushButton { background-color: #333; color: white; border: none; "
      "padding: 12px 30px; border-radius: 8px; font-size: 14px; font-weight: "
      "bold; }"
      "QPushButton:hover { background-color: #444; }");
  connect(clearBtn, &QPushButton::clicked, this, &ExpanderSeats::clearAll);
  btnLayout->addWidget(clearBtn);

  btnLayout->addStretch();
  mainLayout->addLayout(btnLayout);

  QLabel *outputLabel = new QLabel("Expanded Seats:");
  outputLabel->setStyleSheet("font-size: 14px; margin-top: 10px;");
  mainLayout->addWidget(outputLabel);

  m_outputText = new QTextEdit();
  m_outputText->setReadOnly(true);
  m_outputText->setStyleSheet(
      "QTextEdit { background-color: #0a0a0a; color: " + Utils::SUCCESS_COLOR +
      "; border: 1px solid #1f1f1f; "
      "border-radius: 8px; padding: 10px; font-family: Consolas; font-size: "
      "13px; }");
  mainLayout->addWidget(m_outputText, 1);
}

void ExpanderSeats::expandSeats() {
  // Security Check
  SecurityManager::instance().checkAndAct();

  QString input = m_inputText->toPlainText().trimmed();
  if (input.isEmpty()) {
    m_outputText->setText("❌ No input provided.");
    return;
  }

  QStringList lines = input.split('\n', Qt::SkipEmptyParts);
  QStringList allExpanded;

  for (const QString &line : lines) {
    QStringList expanded = expandRange(line.trimmed());
    allExpanded.append(expanded);
  }

  m_outputText->setText(allExpanded.join('\n'));
}

void ExpanderSeats::clearAll() {
  m_inputText->clear();
  m_outputText->clear();
}

QStringList ExpanderSeats::expandRange(const QString &line) {
  QStringList result;

  // Helper to add range
  auto addRange = [&](int start, int end, const QString &prefix,
                      const QString &suffix) {
    if (start <= end) {
      for (int i = start; i <= end; ++i)
        result.append(prefix + QString::number(i) + suffix);
    } else {
      for (int i = start; i >= end; --i)
        result.append(prefix + QString::number(i) + suffix);
    }
  };

  // Pattern 1: "From 10S to 25S" or "Row 1 From 10S to 25S"
  QRegularExpression fromToPattern(
      R"((.*?)(?:From\s+)?(\d+)([A-Za-z]*)\s+to\s+(\d+)([A-Za-z]*))",
      QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch match = fromToPattern.match(line);

  if (match.hasMatch()) {
    QString prefix = match.captured(1).trimmed();
    int start = match.captured(2).toInt();
    QString startSuffix = match.captured(3);
    int end = match.captured(4).toInt();

    if (!prefix.isEmpty() && !prefix.endsWith("-") && !prefix.endsWith(" ")) {
      // Python logic didn't explicitely add separator for From/To case but
      // usually user includes space "Row 1 " Let's add space if not present,
      // unless user typed "Row1"
      if (prefix.back().isLetterOrNumber())
        prefix += " ";
    }

    addRange(start, end, prefix, startSuffix);
    return result;
  }

  // Pattern 2: Slash "10S / 20S"
  QRegularExpression slashPattern(R"((.*?)(\d+)([A-Za-z]*)\s*\/\s*(\d+)\3)",
                                  QRegularExpression::CaseInsensitiveOption);
  match = slashPattern.match(line);
  if (match.hasMatch()) {
    QString prefix = match.captured(1).trimmed();
    int start = match.captured(2).toInt();
    QString suffix = match.captured(3).toUpper();
    int end = match.captured(4).toInt();
    if (!prefix.isEmpty() && !prefix.endsWith("-"))
      prefix += "-";
    addRange(start, end, prefix, suffix);
    return result;
  }

  // Pattern 3: "Row 5 Seats 1-10" or "Seats 1-10" or "100-120"
  QRegularExpression rangePattern(
      R"((?:Row\s+\d+\s+)?(?:Seats?)?\s*(\d+)[-–](\d+))",
      QRegularExpression::CaseInsensitiveOption);
  match = rangePattern.match(line);

  if (match.hasMatch()) {
    int start = match.captured(1).toInt();
    int end = match.captured(2).toInt();
    addRange(start, end, "", "");
    return result;
  }

  // Pattern 4: Single Value matches "10S"
  QRegularExpression singlePattern(R"(.*\d+[A-Za-z])");
  if (singlePattern.match(line).hasMatch()) {
    result.append(line.trimmed().toUpper());
    return result;
  }

  // Fallback: return as-is
  result.append(line);
  return result;
}

} // namespace GOL
