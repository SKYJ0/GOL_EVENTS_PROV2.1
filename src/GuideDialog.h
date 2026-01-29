#ifndef GUIDEDIALOG_H
#define GUIDEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStringList>

namespace GOL {

struct GuideStep {
    QString imagePath;
    QString description;
};

class GuideDialog : public QDialog {
    Q_OBJECT
public:
    explicit GuideDialog(const QString& toolName, const QString& displayName, QWidget* parent = nullptr);

private slots:
    void nextStep();
    void prevStep();
    void updateContent();

private:
    void loadSteps(const QString& toolName);
    void setupUI(const QString& displayName);

    QList<GuideStep> m_steps;
    int m_currentIndex;

    QLabel* m_imageLabel;
    QLabel* m_descLabel;
    QLabel* m_stepCounter;
    QPushButton* m_prevBtn;
    QPushButton* m_nextBtn;
};

} // namespace GOL

#endif // GUIDEDIALOG_H
