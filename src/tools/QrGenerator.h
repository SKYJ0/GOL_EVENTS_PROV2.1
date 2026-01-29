#ifndef QRGENERATOR_H
#define QRGENERATOR_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

namespace GOL {

class QrGenerator : public QDialog {
    Q_OBJECT

public:
    explicit QrGenerator(QWidget* parent = nullptr);

private slots:
    void selectFolder();
    void generateQRCodes();

private:
    QTextEdit* m_inputText;
    QLabel* m_statusLabel;
    QString m_outputFolder;
};

} // namespace GOL

#endif // QRGENERATOR_H
