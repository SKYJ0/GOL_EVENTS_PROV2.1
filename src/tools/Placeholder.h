#ifndef PLACEHOLDER_H
#define PLACEHOLDER_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>

namespace GOL {

class Placeholder : public QDialog {
    Q_OBJECT

public:
    explicit Placeholder(QWidget* parent = nullptr);

private slots:
    void processGeneration();
    void clearAll();

private:
    void log(const QString& msg, bool clear = false);
    
    QTextEdit* m_inputArea;
    QTextEdit* m_logArea;
    QPushButton* m_btnGenerate;
    QPushButton* m_btnClear;
};

} // namespace GOL

#endif // PLACEHOLDER_H
