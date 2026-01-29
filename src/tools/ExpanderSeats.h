#ifndef EXPANDERSEATS_H
#define EXPANDERSEATS_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>

namespace GOL {

class ExpanderSeats : public QDialog {
    Q_OBJECT

public:
    explicit ExpanderSeats(QWidget* parent = nullptr);

private slots:
    void expandSeats();
    void clearAll();

private:
    QTextEdit* m_inputText;
    QTextEdit* m_outputText;
    
    QStringList expandRange(const QString& line);
};

} // namespace GOL

#endif // EXPANDERSEATS_H
