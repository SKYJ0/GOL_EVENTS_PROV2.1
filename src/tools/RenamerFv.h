#ifndef RENAMERFV_H
#define RENAMERFV_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>

namespace GOL {

class RenamerFv : public QDialog {
    Q_OBJECT

public:
    explicit RenamerFv(QWidget* parent = nullptr);

private slots:
    void browseFolder();
    void startProcess();

private:
    void log(const QString& msg);
    QString extractFaceValue(const QString& pdfPath);
    
    QLineEdit* m_pathEdit;
    QPushButton* m_btnBrowse;
    QPushButton* m_btnRun;
    QTextEdit* m_logArea;
};

} // namespace GOL

#endif // RENAMERFV_H
