#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include "io/MeshExporter.h"

namespace MetaVisage {

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(QWidget* parent = nullptr);
    ~ExportDialog();

    ExportOptions GetOptions() const;
    QString GetFilePath() const { return filepath_; }

private slots:
    void OnBrowse();
    void OnFormatChanged(int index);
    void OnExport();

private:
    void CreateUI();
    void UpdateFileExtension();

    // UI elements
    QLineEdit* filepathEdit_;
    QPushButton* browseButton_;
    QComboBox* formatCombo_;
    QCheckBox* triangulateCheck_;
    QCheckBox* materialsCheck_;
    QCheckBox* applyTransformCheck_;
    QCheckBox* yUpCheck_;
    QDoubleSpinBox* scaleSpinBox_;
    QLineEdit* prefixEdit_;
    QPushButton* exportButton_;
    QPushButton* cancelButton_;

    QString filepath_;
};

} // namespace MetaVisage

#endif // EXPORTDIALOG_H
