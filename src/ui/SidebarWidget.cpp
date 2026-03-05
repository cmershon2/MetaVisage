#include "ui/SidebarWidget.h"
#include "core/Project.h"
#include <QGroupBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QScrollArea>
#include <QCheckBox>

namespace MetaVisage {

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent),
      project_(nullptr),
      layout_(nullptr),
      stageLabel_(nullptr),
      nextStageButton_(nullptr),
      controlsWidget_(nullptr),
      transformModeLabel_(nullptr),
      posXSpinBox_(nullptr),
      posYSpinBox_(nullptr),
      posZSpinBox_(nullptr),
      rotXSpinBox_(nullptr),
      rotYSpinBox_(nullptr),
      rotZSpinBox_(nullptr),
      scaleXSpinBox_(nullptr),
      scaleYSpinBox_(nullptr),
      scaleZSpinBox_(nullptr),
      resetTransformButton_(nullptr),
      targetPointCountLabel_(nullptr),
      morphPointCountLabel_(nullptr),
      matchStatusLabel_(nullptr),
      pointListScroll_(nullptr),
      pointListContent_(nullptr),
      clearAllPointsButton_(nullptr),
      pointSizeSlider_(nullptr),
      symmetryCheckBox_(nullptr),
      symmetryAxisCombo_(nullptr),
      updatingTransformDisplay_(false),
      selectedPointIndex_(-1) {

    setStyleSheet("QWidget { background-color: #2C3E50; color: white; }");

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(16, 16, 16, 16);
    layout_->setSpacing(12);

    // Stage indicator
    stageLabel_ = new QLabel("Stage 1/4: Alignment");
    stageLabel_->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; }");
    layout_->addWidget(stageLabel_);

    // Controls area (will be populated based on stage)
    controlsWidget_ = new QWidget();
    layout_->addWidget(controlsWidget_);

    // Spacer
    layout_->addStretch();

    // Next stage button
    nextStageButton_ = new QPushButton("Next Stage");
    nextStageButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498DB;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    font-size: 11pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980B9;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #555;"
        "}"
    );
    nextStageButton_->setEnabled(false);
    connect(nextStageButton_, &QPushButton::clicked, this, &SidebarWidget::NextStageRequested);
    layout_->addWidget(nextStageButton_);

    SetStage(WorkflowStage::Alignment);
}

SidebarWidget::~SidebarWidget() {
}

void SidebarWidget::SetStage(WorkflowStage stage) {
    ClearControls();

    switch (stage) {
        case WorkflowStage::Alignment:
            stageLabel_->setText("Stage 1/4: Alignment");
            CreateAlignmentControls();
            break;

        case WorkflowStage::PointReference:
            stageLabel_->setText("Stage 2/4: Point Reference");
            CreatePointReferenceControls();
            break;

        case WorkflowStage::Morph:
            stageLabel_->setText("Stage 3/4: Morph");
            CreateMorphControls();
            break;

        case WorkflowStage::TouchUp:
            stageLabel_->setText("Stage 4/4: Touch Up");
            CreateTouchUpControls();
            break;
    }
}

void SidebarWidget::CreateAlignmentControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(16);

    QLabel* info = new QLabel("Load both meshes using File menu (Ctrl+M for Morph, Ctrl+T for Target)");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);

    // Mesh Status Group
    QGroupBox* meshStatusGroup = new QGroupBox("Mesh Status");
    QVBoxLayout* meshStatusLayout = new QVBoxLayout(meshStatusGroup);

    QLabel* morphStatus = new QLabel("Morph Mesh: Not Loaded");
    morphStatus->setObjectName("morphStatus");
    morphStatus->setStyleSheet("QLabel { color: #E74C3C; }"); // Red
    meshStatusLayout->addWidget(morphStatus);

    QLabel* targetStatus = new QLabel("Target Mesh: Not Loaded");
    targetStatus->setObjectName("targetStatus");
    targetStatus->setStyleSheet("QLabel { color: #E74C3C; }"); // Red
    meshStatusLayout->addWidget(targetStatus);

    controlsLayout->addWidget(meshStatusGroup);

    // Shading Mode Group
    QGroupBox* shadingGroup = new QGroupBox("Shading Mode");
    QVBoxLayout* shadingLayout = new QVBoxLayout(shadingGroup);

    QComboBox* shadingCombo = new QComboBox();
    shadingCombo->addItem("Solid", static_cast<int>(ShadingMode::Solid));
    shadingCombo->addItem("Wireframe", static_cast<int>(ShadingMode::Wireframe));
    shadingCombo->addItem("Solid + Wireframe", static_cast<int>(ShadingMode::SolidWireframe));
    shadingCombo->setCurrentIndex(0);
    shadingLayout->addWidget(shadingCombo);

    controlsLayout->addWidget(shadingGroup);

    // Transform Tools Group
    QGroupBox* toolsGroup = new QGroupBox("Transform Tools");
    QVBoxLayout* toolsLayout = new QVBoxLayout(toolsGroup);

    // Tool instructions
    QLabel* toolsInfo = new QLabel(
        "G - Move | R - Rotate | S - Scale\n"
        "X/Y/Z - Constrain to axis\n"
        "Esc - Cancel | Left-click + drag to apply"
    );
    toolsInfo->setWordWrap(true);
    toolsInfo->setStyleSheet("QLabel { color: #95A5A6; font-size: 9pt; }");
    toolsLayout->addWidget(toolsInfo);

    // Current transform mode display
    transformModeLabel_ = new QLabel("Mode: None");
    transformModeLabel_->setStyleSheet("QLabel { color: #3498DB; font-weight: bold; font-size: 10pt; }");
    toolsLayout->addWidget(transformModeLabel_);

    controlsLayout->addWidget(toolsGroup);

    // Target Transform Group
    QGroupBox* transformGroup = new QGroupBox("Target Transform");
    QGridLayout* transformLayout = new QGridLayout(transformGroup);
    transformLayout->setSpacing(4);
    transformLayout->setColumnStretch(1, 1);
    transformLayout->setColumnStretch(3, 1);
    transformLayout->setColumnStretch(5, 1);

    // Spinbox style for dark theme
    QString spinBoxStyle =
        "QDoubleSpinBox {"
        "    background-color: #34495E;"
        "    color: white;"
        "    border: 1px solid #555;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "}"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
        "    background-color: #3498DB;"
        "    border: none;"
        "    width: 14px;"
        "}"
        "QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {"
        "    background-color: #2980B9;"
        "}";

    auto createPosSpinBox = [&]() -> QDoubleSpinBox* {
        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        spinBox->setRange(-1000.0, 1000.0);
        spinBox->setDecimals(2);
        spinBox->setSingleStep(0.01);
        spinBox->setValue(0.0);
        spinBox->setStyleSheet(spinBoxStyle);
        spinBox->setMinimumWidth(55);
        return spinBox;
    };

    auto createRotSpinBox = [&]() -> QDoubleSpinBox* {
        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        spinBox->setRange(-360.0, 360.0);
        spinBox->setDecimals(1);
        spinBox->setSingleStep(1.0);
        spinBox->setValue(0.0);
        spinBox->setSuffix(QString::fromUtf8("\u00B0"));
        spinBox->setStyleSheet(spinBoxStyle);
        spinBox->setMinimumWidth(55);
        return spinBox;
    };

    auto createScaleSpinBox = [&]() -> QDoubleSpinBox* {
        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        spinBox->setRange(0.001, 100.0);
        spinBox->setDecimals(3);
        spinBox->setSingleStep(0.01);
        spinBox->setValue(1.0);
        spinBox->setStyleSheet(spinBoxStyle);
        spinBox->setMinimumWidth(55);
        return spinBox;
    };

    int row = 0;

    // Position row
    QLabel* posLabel = new QLabel("Pos:");
    posLabel->setStyleSheet("QLabel { font-weight: bold; }");
    transformLayout->addWidget(posLabel, row, 0);

    QLabel* posXLabel = new QLabel("X");
    posXLabel->setStyleSheet("QLabel { color: #E74C3C; }");
    posXLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(posXLabel, row, 1);
    posXSpinBox_ = createPosSpinBox();
    transformLayout->addWidget(posXSpinBox_, row, 2);

    QLabel* posYLabel = new QLabel("Y");
    posYLabel->setStyleSheet("QLabel { color: #2ECC71; }");
    posYLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(posYLabel, row, 3);
    posYSpinBox_ = createPosSpinBox();
    transformLayout->addWidget(posYSpinBox_, row, 4);

    QLabel* posZLabel = new QLabel("Z");
    posZLabel->setStyleSheet("QLabel { color: #3498DB; }");
    posZLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(posZLabel, row, 5);
    posZSpinBox_ = createPosSpinBox();
    transformLayout->addWidget(posZSpinBox_, row, 6);
    row++;

    // Rotation row
    QLabel* rotLabel = new QLabel("Rot:");
    rotLabel->setStyleSheet("QLabel { font-weight: bold; }");
    transformLayout->addWidget(rotLabel, row, 0);
    QLabel* rotXLabel = new QLabel("X");
    rotXLabel->setStyleSheet("QLabel { color: #E74C3C; }");
    rotXLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(rotXLabel, row, 1);
    rotXSpinBox_ = createRotSpinBox();
    transformLayout->addWidget(rotXSpinBox_, row, 2);
    QLabel* rotYLabel = new QLabel("Y");
    rotYLabel->setStyleSheet("QLabel { color: #2ECC71; }");
    rotYLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(rotYLabel, row, 3);
    rotYSpinBox_ = createRotSpinBox();
    transformLayout->addWidget(rotYSpinBox_, row, 4);
    QLabel* rotZLabel = new QLabel("Z");
    rotZLabel->setStyleSheet("QLabel { color: #3498DB; }");
    rotZLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(rotZLabel, row, 5);
    rotZSpinBox_ = createRotSpinBox();
    transformLayout->addWidget(rotZSpinBox_, row, 6);
    row++;

    // Scale row
    QLabel* scaleLabel = new QLabel("Scale:");
    scaleLabel->setStyleSheet("QLabel { font-weight: bold; }");
    transformLayout->addWidget(scaleLabel, row, 0);
    QLabel* scaleXLabel = new QLabel("X");
    scaleXLabel->setStyleSheet("QLabel { color: #E74C3C; }");
    scaleXLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(scaleXLabel, row, 1);
    scaleXSpinBox_ = createScaleSpinBox();
    transformLayout->addWidget(scaleXSpinBox_, row, 2);
    QLabel* scaleYLabel = new QLabel("Y");
    scaleYLabel->setStyleSheet("QLabel { color: #2ECC71; }");
    scaleYLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(scaleYLabel, row, 3);
    scaleYSpinBox_ = createScaleSpinBox();
    transformLayout->addWidget(scaleYSpinBox_, row, 4);
    QLabel* scaleZLabel = new QLabel("Z");
    scaleZLabel->setStyleSheet("QLabel { color: #3498DB; }");
    scaleZLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    transformLayout->addWidget(scaleZLabel, row, 5);
    scaleZSpinBox_ = createScaleSpinBox();
    transformLayout->addWidget(scaleZSpinBox_, row, 6);
    row++;

    // Connect spinbox value changes
    connect(posXSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(posYSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(posZSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(rotXSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(rotYSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(rotZSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(scaleXSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(scaleYSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });
    connect(scaleZSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { OnSpinBoxValueChanged(); });

    // Reset button
    resetTransformButton_ = new QPushButton("Reset Transform");
    resetTransformButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #E74C3C;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    font-size: 10pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #C0392B;"
        "}"
    );
    connect(resetTransformButton_, &QPushButton::clicked, this, &SidebarWidget::ResetTransformRequested);
    transformLayout->addWidget(resetTransformButton_, row, 0, 1, 7);

    controlsLayout->addWidget(transformGroup);
    controlsLayout->addStretch();

    UpdateTransformDisplay();
}

void SidebarWidget::CreatePointReferenceControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(16);

    QLabel* info = new QLabel(
        "Click on mesh surfaces to place correspondence points.\n\n"
        "Place matching points on the target mesh (left) and morph mesh (right). "
        "Points are auto-numbered sequentially."
    );
    info->setWordWrap(true);
    info->setStyleSheet("QLabel { color: #BDC3C7; }");
    controlsLayout->addWidget(info);

    // Point Count Group
    QGroupBox* countGroup = new QGroupBox("Point Counts");
    countGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* countLayout = new QVBoxLayout(countGroup);

    targetPointCountLabel_ = new QLabel("Target Points: 0");
    targetPointCountLabel_->setStyleSheet("QLabel { color: #F39C12; font-size: 10pt; }");
    countLayout->addWidget(targetPointCountLabel_);

    morphPointCountLabel_ = new QLabel("Morph Points: 0");
    morphPointCountLabel_->setStyleSheet("QLabel { color: #3498DB; font-size: 10pt; }");
    countLayout->addWidget(morphPointCountLabel_);

    matchStatusLabel_ = new QLabel("No points placed");
    matchStatusLabel_->setStyleSheet("QLabel { color: #95A5A6; font-weight: bold; font-size: 10pt; }");
    matchStatusLabel_->setAlignment(Qt::AlignCenter);
    countLayout->addWidget(matchStatusLabel_);

    controlsLayout->addWidget(countGroup);

    // Point List Group
    QGroupBox* listGroup = new QGroupBox("Correspondence Points");
    listGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    pointListScroll_ = new QScrollArea();
    pointListScroll_->setWidgetResizable(true);
    pointListScroll_->setMinimumHeight(120);
    pointListScroll_->setMaximumHeight(250);
    pointListScroll_->setStyleSheet(
        "QScrollArea { background-color: #34495E; border: 1px solid #555; border-radius: 3px; }"
        "QScrollBar:vertical { background-color: #2C3E50; width: 10px; }"
        "QScrollBar::handle:vertical { background-color: #555; border-radius: 5px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );

    pointListContent_ = new QWidget();
    QVBoxLayout* pointListLayout = new QVBoxLayout(pointListContent_);
    pointListLayout->setContentsMargins(4, 4, 4, 4);
    QLabel* noPointsLabel = new QLabel("No points placed yet.\nClick on mesh to add points.");
    noPointsLabel->setAlignment(Qt::AlignCenter);
    noPointsLabel->setStyleSheet("QLabel { color: #7F8C8D; font-style: italic; }");
    pointListLayout->addWidget(noPointsLabel);
    pointListLayout->addStretch();
    pointListScroll_->setWidget(pointListContent_);
    listLayout->addWidget(pointListScroll_);

    controlsLayout->addWidget(listGroup);

    // Point Size Group
    QGroupBox* sizeGroup = new QGroupBox("Point Size");
    sizeGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QHBoxLayout* sizeLayout = new QHBoxLayout(sizeGroup);

    pointSizeSlider_ = new QSlider(Qt::Horizontal);
    pointSizeSlider_->setRange(4, 30);
    pointSizeSlider_->setValue(12);
    pointSizeSlider_->setStyleSheet(
        "QSlider::groove:horizontal { background: #34495E; height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #3498DB; width: 14px; margin: -4px 0; border-radius: 7px; }"
    );
    connect(pointSizeSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnPointSizeChanged);
    sizeLayout->addWidget(pointSizeSlider_);

    QLabel* sizeValueLabel = new QLabel("12");
    sizeValueLabel->setObjectName("pointSizeValue");
    sizeValueLabel->setMinimumWidth(25);
    sizeLayout->addWidget(sizeValueLabel);

    controlsLayout->addWidget(sizeGroup);

    // Actions Group
    QGroupBox* actionsGroup = new QGroupBox("Actions");
    actionsGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup);

    clearAllPointsButton_ = new QPushButton("Clear All Points");
    clearAllPointsButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #E74C3C;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    font-size: 10pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #C0392B;"
        "}"
    );
    connect(clearAllPointsButton_, &QPushButton::clicked, this, &SidebarWidget::ClearAllPointsRequested);
    actionsLayout->addWidget(clearAllPointsButton_);

    QLabel* deleteInfo = new QLabel("Delete - Remove selected point\nEsc - Deselect point");
    deleteInfo->setWordWrap(true);
    deleteInfo->setStyleSheet("QLabel { color: #95A5A6; font-size: 8pt; }");
    actionsLayout->addWidget(deleteInfo);

    controlsLayout->addWidget(actionsGroup);

    // Symmetry Group
    QGroupBox* symmetryGroup = new QGroupBox("Symmetry");
    symmetryGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* symmetryLayout = new QVBoxLayout(symmetryGroup);

    symmetryCheckBox_ = new QCheckBox("Enable Symmetry Mode");
    symmetryCheckBox_->setStyleSheet("QCheckBox { color: white; }");
    connect(symmetryCheckBox_, &QCheckBox::toggled, this, &SidebarWidget::OnSymmetryToggled);
    symmetryLayout->addWidget(symmetryCheckBox_);

    QHBoxLayout* axisLayout = new QHBoxLayout();
    QLabel* axisLabel = new QLabel("Axis:");
    axisLabel->setStyleSheet("QLabel { color: #BDC3C7; }");
    axisLayout->addWidget(axisLabel);

    symmetryAxisCombo_ = new QComboBox();
    symmetryAxisCombo_->addItem("X", static_cast<int>(Axis::X));
    symmetryAxisCombo_->addItem("Y", static_cast<int>(Axis::Y));
    symmetryAxisCombo_->addItem("Z", static_cast<int>(Axis::Z));
    symmetryAxisCombo_->setCurrentIndex(0);
    symmetryAxisCombo_->setEnabled(false);
    symmetryAxisCombo_->setStyleSheet(
        "QComboBox { background-color: #34495E; color: white; border: 1px solid #555; padding: 3px; }"
        "QComboBox:disabled { color: #7F8C8D; }"
    );
    connect(symmetryAxisCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SidebarWidget::OnSymmetryAxisChanged);
    axisLayout->addWidget(symmetryAxisCombo_);
    axisLayout->addStretch();
    symmetryLayout->addLayout(axisLayout);

    QLabel* symmetryNote = new QLabel("Mirror points across the selected axis plane");
    symmetryNote->setStyleSheet("QLabel { color: #7F8C8D; font-size: 8pt; font-style: italic; }");
    symmetryNote->setWordWrap(true);
    symmetryLayout->addWidget(symmetryNote);

    controlsLayout->addWidget(symmetryGroup);

    controlsLayout->addStretch();

    nextStageButton_->setEnabled(false);
}

void SidebarWidget::CreateMorphControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    QLabel* info = new QLabel("Adjust parameters and process the morph.");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);
}

void SidebarWidget::CreateTouchUpControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    QLabel* info = new QLabel("Refine the mesh with sculpting tools.");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);
}

void SidebarWidget::ClearControls() {
    transformModeLabel_ = nullptr;
    posXSpinBox_ = nullptr; posYSpinBox_ = nullptr; posZSpinBox_ = nullptr;
    rotXSpinBox_ = nullptr; rotYSpinBox_ = nullptr; rotZSpinBox_ = nullptr;
    scaleXSpinBox_ = nullptr; scaleYSpinBox_ = nullptr; scaleZSpinBox_ = nullptr;
    resetTransformButton_ = nullptr;
    targetPointCountLabel_ = nullptr; morphPointCountLabel_ = nullptr;
    matchStatusLabel_ = nullptr; pointListScroll_ = nullptr; pointListContent_ = nullptr;
    clearAllPointsButton_ = nullptr; pointSizeSlider_ = nullptr;
    symmetryCheckBox_ = nullptr; symmetryAxisCombo_ = nullptr;
    selectedPointIndex_ = -1;

    if (controlsWidget_->layout()) {
        QLayoutItem* item;
        while ((item = controlsWidget_->layout()->takeAt(0))) {
            delete item->widget();
            delete item;
        }
        delete controlsWidget_->layout();
    }
}

void SidebarWidget::UpdatePointCounts() {
    if (!project_ || !targetPointCountLabel_ || !morphPointCountLabel_ || !matchStatusLabel_) return;

    const auto& correspondences = project_->GetPointReferenceData().correspondences;
    int targetCount = 0, morphCount = 0;
    for (const auto& corr : correspondences) {
        if (corr.targetMeshVertexIndex >= 0) targetCount++;
        if (corr.morphMeshVertexIndex >= 0) morphCount++;
    }

    targetPointCountLabel_->setText(QString("Target Points: %1").arg(targetCount));
    morphPointCountLabel_->setText(QString("Morph Points: %1").arg(morphCount));

    if (targetCount == 0 && morphCount == 0) {
        matchStatusLabel_->setText("No points placed");
        matchStatusLabel_->setStyleSheet("QLabel { color: #95A5A6; font-weight: bold; font-size: 10pt; }");
    } else if (targetCount == morphCount) {
        matchStatusLabel_->setText("Points matched!");
        matchStatusLabel_->setStyleSheet("QLabel { color: #2ECC71; font-weight: bold; font-size: 10pt; }");
    } else {
        matchStatusLabel_->setText(QString("Mismatch: %1 vs %2").arg(targetCount).arg(morphCount));
        matchStatusLabel_->setStyleSheet("QLabel { color: #F39C12; font-weight: bold; font-size: 10pt; }");
    }

    nextStageButton_->setEnabled(project_->CanProceedToNextStage());
}

void SidebarWidget::UpdatePointList() {
    RebuildPointListContent();
    UpdatePointCounts();
}

void SidebarWidget::RebuildPointListContent() {
    if (!pointListScroll_ || !project_) return;

    QWidget* newContent = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(newContent);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    const auto& correspondences = project_->GetPointReferenceData().correspondences;

    if (correspondences.empty()) {
        QLabel* noPointsLabel = new QLabel("No points placed yet.\nClick on mesh to add points.");
        noPointsLabel->setAlignment(Qt::AlignCenter);
        noPointsLabel->setStyleSheet("QLabel { color: #7F8C8D; font-style: italic; }");
        layout->addWidget(noPointsLabel);
    } else {
        for (size_t i = 0; i < correspondences.size(); ++i) {
            const auto& corr = correspondences[i];
            int corrIdx = static_cast<int>(i);
            bool isSelected = (corrIdx == selectedPointIndex_);

            bool hasTarget = corr.targetMeshVertexIndex >= 0;
            bool hasMorph = corr.morphMeshVertexIndex >= 0;

            QString status;
            if (hasTarget && hasMorph) status = "Paired";
            else if (hasTarget) status = "Target only";
            else status = "Morph only";

            QPushButton* pointBtn = new QPushButton();
            pointBtn->setText(QString("Point %1 - %2").arg(corr.pointID).arg(status));

            QString bgColor = isSelected ? "#E67E22" : "#34495E";
            QString hoverColor = isSelected ? "#D35400" : "#4A6785";
            pointBtn->setStyleSheet(
                QString("QPushButton { background-color: %1; color: white; border: none; "
                        "padding: 4px 8px; text-align: left; border-radius: 3px; font-size: 9pt; }"
                        "QPushButton:hover { background-color: %2; }").arg(bgColor, hoverColor)
            );

            connect(pointBtn, &QPushButton::clicked, this, [this, corrIdx]() {
                selectedPointIndex_ = corrIdx;
                emit PointSelectedFromList(corrIdx);
                RebuildPointListContent();
            });

            layout->addWidget(pointBtn);
        }
    }

    layout->addStretch();
    pointListContent_ = newContent;
    pointListScroll_->setWidget(newContent);
}

void SidebarWidget::SetSelectedPointIndex(int index) {
    selectedPointIndex_ = index;
    RebuildPointListContent();
}

void SidebarWidget::OnPointSizeChanged(int value) {
    QLabel* sizeValueLabel = findChild<QLabel*>("pointSizeValue");
    if (sizeValueLabel) sizeValueLabel->setText(QString::number(value));
    emit PointSizeChanged(static_cast<float>(value));
}

void SidebarWidget::OnSymmetryToggled(bool enabled) {
    if (symmetryAxisCombo_) symmetryAxisCombo_->setEnabled(enabled);
    if (project_) project_->GetPointReferenceData().symmetryEnabled = enabled;

    Axis axis = Axis::X;
    if (symmetryAxisCombo_) axis = static_cast<Axis>(symmetryAxisCombo_->currentData().toInt());
    emit SymmetryChanged(enabled, axis);
}

void SidebarWidget::OnSymmetryAxisChanged(int index) {
    Q_UNUSED(index);
    if (!symmetryAxisCombo_ || !project_) return;
    Axis axis = static_cast<Axis>(symmetryAxisCombo_->currentData().toInt());
    project_->GetPointReferenceData().symmetryAxis = axis;
    bool enabled = symmetryCheckBox_ ? symmetryCheckBox_->isChecked() : false;
    emit SymmetryChanged(enabled, axis);
}

void SidebarWidget::OnTransformModeChanged(TransformMode mode, AxisConstraint axis) {
    if (!transformModeLabel_) return;

    QString modeStr;
    switch (mode) {
        case TransformMode::Move: modeStr = "Move"; break;
        case TransformMode::Rotate: modeStr = "Rotate"; break;
        case TransformMode::Scale: modeStr = "Scale"; break;
        default: modeStr = "None"; break;
    }

    QString axisStr;
    switch (axis) {
        case AxisConstraint::X: axisStr = " [X only]"; break;
        case AxisConstraint::Y: axisStr = " [Y only]"; break;
        case AxisConstraint::Z: axisStr = " [Z only]"; break;
        default: axisStr = ""; break;
    }

    transformModeLabel_->setText("Mode: " + modeStr + axisStr);
    if (mode == TransformMode::None) {
        transformModeLabel_->setStyleSheet("QLabel { color: #95A5A6; font-weight: bold; font-size: 10pt; }");
    } else {
        transformModeLabel_->setStyleSheet("QLabel { color: #2ECC71; font-weight: bold; font-size: 10pt; }");
    }
}

void SidebarWidget::OnTargetTransformChanged() {
    UpdateTransformDisplay();
}

void SidebarWidget::UpdateTransformDisplay() {
    if (!project_ || !posXSpinBox_ || !posYSpinBox_ || !posZSpinBox_ ||
        !rotXSpinBox_ || !rotYSpinBox_ || !rotZSpinBox_ ||
        !scaleXSpinBox_ || !scaleYSpinBox_ || !scaleZSpinBox_) return;

    updatingTransformDisplay_ = true;

    const Transform& transform = project_->GetTargetMesh().transform;
    Vector3 pos = transform.GetPosition();
    Vector3 euler = transform.GetRotation().ToEulerAngles();
    Vector3 scale = transform.GetScale();

    posXSpinBox_->setValue(pos.x); posYSpinBox_->setValue(pos.y); posZSpinBox_->setValue(pos.z);
    rotXSpinBox_->setValue(euler.x); rotYSpinBox_->setValue(euler.y); rotZSpinBox_->setValue(euler.z);
    scaleXSpinBox_->setValue(scale.x); scaleYSpinBox_->setValue(scale.y); scaleZSpinBox_->setValue(scale.z);

    updatingTransformDisplay_ = false;
}

void SidebarWidget::OnSpinBoxValueChanged() {
    if (updatingTransformDisplay_) return;
    if (!project_ || !project_->GetTargetMesh().isLoaded) return;
    if (!posXSpinBox_ || !posYSpinBox_ || !posZSpinBox_ ||
        !rotXSpinBox_ || !rotYSpinBox_ || !rotZSpinBox_ ||
        !scaleXSpinBox_ || !scaleYSpinBox_ || !scaleZSpinBox_) return;

    Vector3 newPos(static_cast<float>(posXSpinBox_->value()),
                   static_cast<float>(posYSpinBox_->value()),
                   static_cast<float>(posZSpinBox_->value()));
    Quaternion newRot = Quaternion::FromEulerAngles(
        static_cast<float>(rotXSpinBox_->value()),
        static_cast<float>(rotYSpinBox_->value()),
        static_cast<float>(rotZSpinBox_->value()));
    Vector3 newScale(static_cast<float>(scaleXSpinBox_->value()),
                     static_cast<float>(scaleYSpinBox_->value()),
                     static_cast<float>(scaleZSpinBox_->value()));

    Transform& transform = project_->GetTargetMesh().transform;
    transform.SetPosition(newPos);
    transform.SetRotation(newRot);
    transform.SetScale(newScale);

    emit TransformValuesChanged();
}

} // namespace MetaVisage
