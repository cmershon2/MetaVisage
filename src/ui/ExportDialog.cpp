#include "ui/ExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QFileInfo>

namespace MetaVisage {

ExportDialog::ExportDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Export Mesh");
    setMinimumWidth(500);
    CreateUI();
}

ExportDialog::~ExportDialog() {
}

void ExportDialog::CreateUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // File path section
    QGroupBox* fileGroup = new QGroupBox("Output File");
    QHBoxLayout* fileLayout = new QHBoxLayout(fileGroup);
    filepathEdit_ = new QLineEdit();
    filepathEdit_->setPlaceholderText("Select export location...");
    browseButton_ = new QPushButton("Browse...");
    fileLayout->addWidget(filepathEdit_, 1);
    fileLayout->addWidget(browseButton_);
    mainLayout->addWidget(fileGroup);

    // Format & Options section
    QGroupBox* optionsGroup = new QGroupBox("Export Options");
    QFormLayout* optionsLayout = new QFormLayout(optionsGroup);

    formatCombo_ = new QComboBox();
    formatCombo_->addItem("FBX", static_cast<int>(ExportFormat::FBX));
    formatCombo_->addItem("OBJ", static_cast<int>(ExportFormat::OBJ));
    formatCombo_->addItem("GLTF", static_cast<int>(ExportFormat::GLTF));
    optionsLayout->addRow("Format:", formatCombo_);

    triangulateCheck_ = new QCheckBox("Triangulate mesh");
    triangulateCheck_->setChecked(true);
    optionsLayout->addRow("", triangulateCheck_);

    materialsCheck_ = new QCheckBox("Include materials");
    materialsCheck_->setChecked(true);
    optionsLayout->addRow("", materialsCheck_);

    applyTransformCheck_ = new QCheckBox("Bake transform into vertices");
    applyTransformCheck_->setChecked(true);
    optionsLayout->addRow("", applyTransformCheck_);

    yUpCheck_ = new QCheckBox("Y-Up coordinate system (Unreal Engine)");
    yUpCheck_->setChecked(true);
    optionsLayout->addRow("", yUpCheck_);

    metaHumanCheck_ = new QCheckBox("MetaHuman compatible (preserves original mesh topology)");
    metaHumanCheck_->setChecked(false);
    metaHumanCheck_->setToolTip("Rewrites the original morph mesh file with deformed positions,\n"
                                 "preserving the exact vertex count and face connectivity required\n"
                                 "by UE5 MetaHuman Conform. Format is auto-detected from the original file.");
    optionsLayout->addRow("", metaHumanCheck_);

    scaleSpinBox_ = new QDoubleSpinBox();
    scaleSpinBox_->setRange(0.001, 1000.0);
    scaleSpinBox_->setValue(1.0);
    scaleSpinBox_->setDecimals(3);
    scaleSpinBox_->setSingleStep(0.1);
    optionsLayout->addRow("Scale Factor:", scaleSpinBox_);

    prefixEdit_ = new QLineEdit();
    prefixEdit_->setPlaceholderText("e.g. SK_ for skeletal mesh");
    optionsLayout->addRow("UE Name Prefix:", prefixEdit_);

    mainLayout->addWidget(optionsGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    cancelButton_ = new QPushButton("Cancel");
    exportButton_ = new QPushButton("Export");
    exportButton_->setDefault(true);
    exportButton_->setEnabled(false);
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(exportButton_);
    mainLayout->addLayout(buttonLayout);

    // Style
    QString buttonStyle = "QPushButton { padding: 8px 24px; }";
    exportButton_->setStyleSheet(buttonStyle + " QPushButton { background-color: #3498DB; color: white; font-weight: bold; }");
    cancelButton_->setStyleSheet(buttonStyle);

    // Connections
    connect(browseButton_, &QPushButton::clicked, this, &ExportDialog::OnBrowse);
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportDialog::OnFormatChanged);
    connect(exportButton_, &QPushButton::clicked, this, &ExportDialog::OnExport);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    connect(filepathEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        exportButton_->setEnabled(!text.isEmpty());
    });
}

void ExportDialog::OnBrowse() {
    ExportFormat format = static_cast<ExportFormat>(formatCombo_->currentData().toInt());
    QString filter = MeshExporter::GetFormatFilter(format);
    QString ext = MeshExporter::GetFormatExtension(format);

    QString filepath = QFileDialog::getSaveFileName(
        this, "Export Mesh", QString(), MeshExporter::GetAllFormatsFilter());

    if (!filepath.isEmpty()) {
        // Ensure correct extension
        if (!filepath.endsWith(ext, Qt::CaseInsensitive)) {
            filepath += ext;
        }
        filepathEdit_->setText(filepath);
        filepath_ = filepath;

        // Update format combo to match selected extension
        if (filepath.endsWith(".obj", Qt::CaseInsensitive)) {
            formatCombo_->setCurrentIndex(1);
        } else if (filepath.endsWith(".gltf", Qt::CaseInsensitive)) {
            formatCombo_->setCurrentIndex(2);
        } else {
            formatCombo_->setCurrentIndex(0);
        }
    }
}

void ExportDialog::OnFormatChanged(int /*index*/) {
    UpdateFileExtension();
}

void ExportDialog::OnExport() {
    filepath_ = filepathEdit_->text();
    if (!filepath_.isEmpty()) {
        accept();
    }
}

void ExportDialog::UpdateFileExtension() {
    QString currentPath = filepathEdit_->text();
    if (currentPath.isEmpty()) return;

    ExportFormat format = static_cast<ExportFormat>(formatCombo_->currentData().toInt());
    QString newExt = MeshExporter::GetFormatExtension(format);

    // Replace extension
    QFileInfo info(currentPath);
    QString basePath = info.absolutePath() + "/" + info.completeBaseName();
    filepathEdit_->setText(basePath + newExt);
    filepath_ = filepathEdit_->text();
}

ExportOptions ExportDialog::GetOptions() const {
    ExportOptions options;
    options.format = static_cast<ExportFormat>(formatCombo_->currentData().toInt());
    options.triangulate = triangulateCheck_->isChecked();
    options.includeMaterials = materialsCheck_->isChecked();
    options.applyTransform = applyTransformCheck_->isChecked();
    options.convertToYUp = yUpCheck_->isChecked();
    options.scaleFactor = static_cast<float>(scaleSpinBox_->value());
    options.ueNamingPrefix = prefixEdit_->text();
    options.metaHumanCompatible = metaHumanCheck_->isChecked();
    options.undoUVFlip = metaHumanCheck_->isChecked();  // Auto-enable UV un-flip for MetaHuman
    return options;
}

} // namespace MetaVisage
