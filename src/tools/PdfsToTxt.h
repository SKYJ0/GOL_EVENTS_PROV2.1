#ifndef PDFSTOTXT_H
#define PDFSTOTXT_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>

namespace GOL {

class PdfsToTxt : public QDialog {
    Q_OBJECT

public:
    explicit PdfsToTxt(QWidget* parent = nullptr);

private slots:
    void browseFolder();
    void runCleaner();

private:
    void log(const QString& msg, bool clear = false);
    
    // Static helper for natural sorting
    static bool naturalSort(const QString& a, const QString& b);

    QLineEdit* m_pathEdit;
    QTextEdit* m_logArea;
    QPushButton* m_btnBrowse;
    QPushButton* m_btnProcess;
};

} // namespace GOL

#endif // PDFSTOTXT_H
