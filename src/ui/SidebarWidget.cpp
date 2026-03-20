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
      prevStageButton_(nullptr),
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
      algorithmCombo_(nullptr),
      rbfParamsWidget_(nullptr),
      nricpParamsWidget_(nullptr),
      stiffnessSlider_(nullptr),
      smoothnessSlider_(nullptr),
      stiffnessValueLabel_(nullptr),
      smoothnessValueLabel_(nullptr),
      kernelTypeCombo_(nullptr),
      nricpAlphaInitialSlider_(nullptr),
      nricpAlphaFinalSlider_(nullptr),
      nricpStiffnessStepsSlider_(nullptr),
      nricpIcpIterationsSlider_(nullptr),
      nricpNormalThresholdSlider_(nullptr),
      nricpLandmarkWeightSlider_(nullptr),
      nricpAlphaInitialLabel_(nullptr),
      nricpAlphaFinalLabel_(nullptr),
      nricpStiffnessStepsLabel_(nullptr),
      nricpIcpIterationsLabel_(nullptr),
      nricpNormalThresholdLabel_(nullptr),
      nricpLandmarkWeightLabel_(nullptr),
      nricpBoundaryExclusionCheckBox_(nullptr),
      nricpBoundaryHopsSlider_(nullptr),
      nricpBoundaryHopsLabel_(nullptr),
      processButton_(nullptr),
      cancelButton_(nullptr),
      progressBar_(nullptr),
      progressLabel_(nullptr),
      previewModeCombo_(nullptr),
      acceptButton_(nullptr),
      reprocessButton_(nullptr),
      resetDefaultsButton_(nullptr),
      brushButtonGroup_(nullptr),
      smoothBrushButton_(nullptr),
      grabBrushButton_(nullptr),
      pushPullBrushButton_(nullptr),
      inflateBrushButton_(nullptr),
      brushRadiusSlider_(nullptr),
      brushRadiusValueLabel_(nullptr),
      brushStrengthSlider_(nullptr),
      brushStrengthValueLabel_(nullptr),
      falloffTypeCombo_(nullptr),
      sculptSymmetryCheckBox_(nullptr),
      sculptSymmetryAxisCombo_(nullptr),
      showOverlayCheckBox_(nullptr),
      finalizeButton_(nullptr),
      updatingTransformDisplay_(false),
      selectedPointIndex_(-1) {

    setStyleSheet("QWidget { background-color: #1E1E1E; color: white; }");

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

    // Stage navigation buttons
    QHBoxLayout* navLayout = new QHBoxLayout();
    navLayout->setSpacing(8);

    prevStageButton_ = new QPushButton("Previous");
    prevStageButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #7F8C8D;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    font-size: 11pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #95A5A6;"
        "}"
    );
    prevStageButton_->setVisible(false); // Hidden on Stage 1
    connect(prevStageButton_, &QPushButton::clicked, this, &SidebarWidget::PreviousStageRequested);
    navLayout->addWidget(prevStageButton_);

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
    navLayout->addWidget(nextStageButton_);

    layout_->addLayout(navLayout);

    SetStage(WorkflowStage::Alignment);
}

SidebarWidget::~SidebarWidget() {
}

void SidebarWidget::SetStage(WorkflowStage stage) {
    ClearControls();
    nextStageButton_->setVisible(true); // Re-show; TouchUp hides it
    prevStageButton_->setVisible(stage != WorkflowStage::Alignment);

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

    // Import Buttons
    QGroupBox* importGroup = new QGroupBox("Import Meshes");
    QVBoxLayout* importLayout = new QVBoxLayout(importGroup);

    QPushButton* importMorphButton = new QPushButton("Import Morph Mesh (MetaHuman)");
    importMorphButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498DB;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 12px;"
        "    font-size: 10pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980B9;"
        "}"
    );
    connect(importMorphButton, &QPushButton::clicked, this, &SidebarWidget::ImportMorphMeshRequested);
    importLayout->addWidget(importMorphButton);

    QPushButton* useDefaultButton = new QPushButton("Use Default MetaHuman Head");
    useDefaultButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2ECC71;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    font-size: 9pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #27AE60;"
        "}"
    );
    connect(useDefaultButton, &QPushButton::clicked, this, &SidebarWidget::UseDefaultMorphMeshRequested);
    importLayout->addWidget(useDefaultButton);

    QPushButton* importTargetButton = new QPushButton("Import Target Mesh (Custom)");
    importTargetButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498DB;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 12px;"
        "    font-size: 10pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980B9;"
        "}"
    );
    connect(importTargetButton, &QPushButton::clicked, this, &SidebarWidget::ImportTargetMeshRequested);
    importLayout->addWidget(importTargetButton);

    controlsLayout->addWidget(importGroup);

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

    // Add tooltips to transform spinboxes
    posXSpinBox_->setToolTip("Target mesh X position");
    posYSpinBox_->setToolTip("Target mesh Y position");
    posZSpinBox_->setToolTip("Target mesh Z position");
    rotXSpinBox_->setToolTip("Target mesh X rotation (degrees)");
    rotYSpinBox_->setToolTip("Target mesh Y rotation (degrees)");
    rotZSpinBox_->setToolTip("Target mesh Z rotation (degrees)");
    scaleXSpinBox_->setToolTip("Target mesh X scale");
    scaleYSpinBox_->setToolTip("Target mesh Y scale");
    scaleZSpinBox_->setToolTip("Target mesh Z scale");
    shadingCombo->setToolTip("Change mesh rendering mode");

    // Reset button
    resetTransformButton_ = new QPushButton("Reset Transform");
    resetTransformButton_->setToolTip("Reset target mesh transform to identity");
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
    pointSizeSlider_->setToolTip("Adjust correspondence point display size");
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
    clearAllPointsButton_->setToolTip("Remove all correspondence points from both meshes");
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
    symmetryCheckBox_->setToolTip("Automatically place mirrored points across the symmetry axis");
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
    controlsLayout->setSpacing(16);

    // Info label
    QLabel* info = new QLabel(
        "Select algorithm and adjust parameters. "
        "NRICP provides best results for face wrapping."
    );
    info->setWordWrap(true);
    info->setStyleSheet("QLabel { color: #BDC3C7; }");
    controlsLayout->addWidget(info);

    // Common styles
    QString groupBoxStyle =
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }";
    QString sliderStyle =
        "QSlider::groove:horizontal { background: #34495E; height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #3498DB; width: 14px; margin: -4px 0; border-radius: 7px; }";
    QString comboStyle =
        "QComboBox { background-color: #34495E; color: white; border: 1px solid #555; padding: 4px; border-radius: 3px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #34495E; color: white; selection-background-color: #3498DB; }";

    // --- Algorithm Selection ---
    QGroupBox* algoGroup = new QGroupBox("Algorithm");
    algoGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* algoLayout = new QVBoxLayout(algoGroup);

    algorithmCombo_ = new QComboBox();
    algorithmCombo_->setToolTip("Select the deformation algorithm");
    algorithmCombo_->addItem("NRICP (Recommended)", static_cast<int>(DeformationAlgorithm::NRICP));
    algorithmCombo_->addItem("RBF - Thin-Plate Spline", static_cast<int>(DeformationAlgorithm::RBF_TPS));
    algorithmCombo_->addItem("RBF - Gaussian", static_cast<int>(DeformationAlgorithm::RBF_GAUSSIAN));
    algorithmCombo_->addItem("RBF - Multiquadric", static_cast<int>(DeformationAlgorithm::RBF_MULTIQUADRIC));
    algorithmCombo_->setCurrentIndex(0);
    algorithmCombo_->setStyleSheet(comboStyle);
    connect(algorithmCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SidebarWidget::OnAlgorithmChanged);
    algoLayout->addWidget(algorithmCombo_);

    controlsLayout->addWidget(algoGroup);

    // --- NRICP Parameters Group ---
    nricpParamsWidget_ = new QWidget();
    QVBoxLayout* nricpOuterLayout = new QVBoxLayout(nricpParamsWidget_);
    nricpOuterLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* nricpGroup = new QGroupBox("NRICP Parameters");
    nricpGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* nricpLayout = new QVBoxLayout(nricpGroup);

    auto addNRICPSlider = [&](const QString& label, const QString& tooltip,
                              QSlider*& slider, QLabel*& valueLabel,
                              int min, int max, int defaultVal) {
        QHBoxLayout* row = new QHBoxLayout();
        QLabel* lbl = new QLabel(label);
        lbl->setMinimumWidth(80);
        lbl->setMaximumWidth(85);
        lbl->setStyleSheet("QLabel { font-size: 8pt; }");
        row->addWidget(lbl);
        slider = new QSlider(Qt::Horizontal);
        slider->setToolTip(tooltip);
        slider->setRange(min, max);
        slider->setValue(defaultVal);
        slider->setStyleSheet(sliderStyle);
        row->addWidget(slider);
        valueLabel = new QLabel();
        valueLabel->setMinimumWidth(35);
        valueLabel->setStyleSheet("QLabel { font-size: 8pt; }");
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(valueLabel);
        nricpLayout->addLayout(row);
    };

    addNRICPSlider("Stiffness Init:", "Initial stiffness (high=rigid start)",
                   nricpAlphaInitialSlider_, nricpAlphaInitialLabel_, 1, 500, 100);
    nricpAlphaInitialLabel_->setText("100");

    addNRICPSlider("Stiffness Final:", "Final stiffness (low=flexible end)",
                   nricpAlphaFinalSlider_, nricpAlphaFinalLabel_, 1, 100, 50);
    nricpAlphaFinalLabel_->setText("50");

    addNRICPSlider("Stiffness Steps:", "Number of coarse-to-fine levels",
                   nricpStiffnessStepsSlider_, nricpStiffnessStepsLabel_, 1, 20, 5);
    nricpStiffnessStepsLabel_->setText("5");

    addNRICPSlider("ICP Iterations:", "ICP iterations per stiffness level",
                   nricpIcpIterationsSlider_, nricpIcpIterationsLabel_, 1, 10, 7);
    nricpIcpIterationsLabel_->setText("7");

    addNRICPSlider("Normal Thresh:", "Max normal angle for correspondence (degrees)",
                   nricpNormalThresholdSlider_, nricpNormalThresholdLabel_, 10, 180, 60);
    nricpNormalThresholdLabel_->setText(QString::fromUtf8("60\u00B0"));

    addNRICPSlider("Landmark Wt:", "Weight for user-defined landmark points",
                   nricpLandmarkWeightSlider_, nricpLandmarkWeightLabel_, 1, 200, 10);
    nricpLandmarkWeightLabel_->setText("10.0");

    // Connect NRICP slider signals
    connect(nricpAlphaInitialSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnNRICPAlphaInitialChanged);
    connect(nricpAlphaFinalSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnNRICPAlphaFinalChanged);
    connect(nricpStiffnessStepsSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnNRICPStiffnessStepsChanged);
    connect(nricpIcpIterationsSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnNRICPIcpIterationsChanged);
    connect(nricpNormalThresholdSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnNRICPNormalThresholdChanged);
    connect(nricpLandmarkWeightSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnNRICPLandmarkWeightChanged);

    // --- Interior Geometry Exclusion ---
    QFrame* nricpSeparator = new QFrame();
    nricpSeparator->setFrameShape(QFrame::HLine);
    nricpSeparator->setStyleSheet("QFrame { color: #555; }");
    nricpLayout->addWidget(nricpSeparator);

    QLabel* boundaryTitle = new QLabel("Interior Geometry");
    boundaryTitle->setStyleSheet("QLabel { font-weight: bold; color: #BDC3C7; }");
    nricpLayout->addWidget(boundaryTitle);

    nricpBoundaryExclusionCheckBox_ = new QCheckBox("Exclude Boundary Regions");
    nricpBoundaryExclusionCheckBox_->setToolTip(
        "Exclude interior geometry (mouth box, eye sockets, ear canals) "
        "from ICP correspondence finding. Prevents interior vertices from "
        "being pulled through the face surface.");
    nricpBoundaryExclusionCheckBox_->setChecked(true);
    nricpLayout->addWidget(nricpBoundaryExclusionCheckBox_);

    QHBoxLayout* hopsRow = new QHBoxLayout();
    QLabel* hopsLbl = new QLabel("Boundary Depth:");
    hopsLbl->setMinimumWidth(95);
    hopsRow->addWidget(hopsLbl);
    nricpBoundaryHopsSlider_ = new QSlider(Qt::Horizontal);
    nricpBoundaryHopsSlider_->setToolTip(
        "Number of edge hops from mesh boundaries to exclude (higher = larger exclusion zone)");
    nricpBoundaryHopsSlider_->setRange(1, 10);
    nricpBoundaryHopsSlider_->setValue(3);
    nricpBoundaryHopsSlider_->setStyleSheet(sliderStyle);
    hopsRow->addWidget(nricpBoundaryHopsSlider_);
    nricpBoundaryHopsLabel_ = new QLabel("3");
    nricpBoundaryHopsLabel_->setMinimumWidth(40);
    nricpBoundaryHopsLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    hopsRow->addWidget(nricpBoundaryHopsLabel_);
    nricpLayout->addLayout(hopsRow);

    connect(nricpBoundaryExclusionCheckBox_, &QCheckBox::toggled,
            this, &SidebarWidget::OnNRICPBoundaryExclusionToggled);
    connect(nricpBoundaryHopsSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPBoundaryHopsChanged);

    // --- Optimization Parameters ---
    QFrame* optSeparator = new QFrame();
    optSeparator->setFrameShape(QFrame::HLine);
    optSeparator->setStyleSheet("QFrame { color: #555; }");
    nricpLayout->addWidget(optSeparator);

    QLabel* optTitle = new QLabel("Optimization");
    optTitle->setStyleSheet("QLabel { font-weight: bold; color: #BDC3C7; }");
    nricpLayout->addWidget(optTitle);

    addNRICPSlider("Opt Iterations:", "Inner optimization iterations per ICP step (1=disabled)",
                   nricpOptimizationIterationsSlider_, nricpOptimizationIterationsLabel_, 1, 20, 1);
    nricpOptimizationIterationsLabel_->setText("1");

    addNRICPSlider("Dp Initial:", "Initial step-size damping (1.0=no damping, lower=more conservative)",
                   nricpDpInitialSlider_, nricpDpInitialLabel_, 1, 100, 100);
    nricpDpInitialLabel_->setText("1.00");

    addNRICPSlider("Dp Final:", "Final step-size damping",
                   nricpDpFinalSlider_, nricpDpFinalLabel_, 1, 100, 100);
    nricpDpFinalLabel_->setText("1.00");

    connect(nricpOptimizationIterationsSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPOptimizationIterationsChanged);
    connect(nricpDpInitialSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPDpInitialChanged);
    connect(nricpDpFinalSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPDpFinalChanged);

    // --- Rigidity Parameters ---
    QFrame* rigidSeparator = new QFrame();
    rigidSeparator->setFrameShape(QFrame::HLine);
    rigidSeparator->setStyleSheet("QFrame { color: #555; }");
    nricpLayout->addWidget(rigidSeparator);

    QLabel* rigidTitle = new QLabel("Rigidity (ARAP)");
    rigidTitle->setStyleSheet("QLabel { font-weight: bold; color: #BDC3C7; }");
    nricpLayout->addWidget(rigidTitle);

    addNRICPSlider("Rigidity Init:", "Initial ARAP rigidity weight (0=disabled, higher=more rigid)",
                   nricpGammaInitialSlider_, nricpGammaInitialLabel_, 0, 100, 0);
    nricpGammaInitialLabel_->setText("0.0");

    addNRICPSlider("Rigidity Final:", "Final ARAP rigidity weight (0=disabled)",
                   nricpGammaFinalSlider_, nricpGammaFinalLabel_, 0, 100, 0);
    nricpGammaFinalLabel_->setText("0.0");

    connect(nricpGammaInitialSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPGammaInitialChanged);
    connect(nricpGammaFinalSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPGammaFinalChanged);

    // --- Control Node Subsampling ---
    QFrame* sampSeparator = new QFrame();
    sampSeparator->setFrameShape(QFrame::HLine);
    sampSeparator->setStyleSheet("QFrame { color: #555; }");
    nricpLayout->addWidget(sampSeparator);

    QLabel* sampTitle = new QLabel("Control Node Sampling");
    sampTitle->setStyleSheet("QLabel { font-weight: bold; color: #BDC3C7; }");
    nricpLayout->addWidget(sampTitle);

    addNRICPSlider("Sampling Init:", "Initial control node density (0=all vertices, higher=fewer nodes)",
                   nricpSamplingInitialSlider_, nricpSamplingInitialLabel_, 0, 100, 100);
    nricpSamplingInitialLabel_->setText("0.100");

    addNRICPSlider("Sampling Final:", "Final control node density (0=all vertices)",
                   nricpSamplingFinalSlider_, nricpSamplingFinalLabel_, 0, 100, 10);
    nricpSamplingFinalLabel_->setText("0.010");

    nricpNormalizeSamplingCheckBox_ = new QCheckBox("Normalize Sampling");
    nricpNormalizeSamplingCheckBox_->setToolTip(
        "Interpret sampling values relative to mesh bounding box diagonal. "
        "If unchecked, values are absolute distances.");
    nricpNormalizeSamplingCheckBox_->setChecked(true);
    nricpLayout->addWidget(nricpNormalizeSamplingCheckBox_);

    connect(nricpSamplingInitialSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPSamplingInitialChanged);
    connect(nricpSamplingFinalSlider_, &QSlider::valueChanged,
            this, &SidebarWidget::OnNRICPSamplingFinalChanged);
    connect(nricpNormalizeSamplingCheckBox_, &QCheckBox::toggled,
            this, &SidebarWidget::OnNRICPNormalizeSamplingToggled);

    nricpOuterLayout->addWidget(nricpGroup);
    controlsLayout->addWidget(nricpParamsWidget_);

    // --- RBF Parameters Group (hidden by default) ---
    rbfParamsWidget_ = new QWidget();
    QVBoxLayout* rbfOuterLayout = new QVBoxLayout(rbfParamsWidget_);
    rbfOuterLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* rbfGroup = new QGroupBox("RBF Parameters");
    rbfGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* rbfLayout = new QVBoxLayout(rbfGroup);

    // Stiffness slider
    QHBoxLayout* stiffLayout = new QHBoxLayout();
    QLabel* stiffLabel = new QLabel("Stiffness:");
    stiffLabel->setMinimumWidth(75);
    stiffLayout->addWidget(stiffLabel);
    stiffnessSlider_ = new QSlider(Qt::Horizontal);
    stiffnessSlider_->setToolTip("Controls mesh rigidity (0=flexible, 1=rigid)");
    stiffnessSlider_->setRange(0, 100);
    stiffnessSlider_->setValue(50);
    stiffnessSlider_->setStyleSheet(sliderStyle);
    stiffLayout->addWidget(stiffnessSlider_);
    stiffnessValueLabel_ = new QLabel("0.50");
    stiffnessValueLabel_->setMinimumWidth(35);
    stiffnessValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    stiffLayout->addWidget(stiffnessValueLabel_);
    rbfLayout->addLayout(stiffLayout);

    // Smoothness slider
    QHBoxLayout* smoothLayout = new QHBoxLayout();
    QLabel* smoothLabel = new QLabel("Smoothness:");
    smoothLabel->setMinimumWidth(75);
    smoothLayout->addWidget(smoothLabel);
    smoothnessSlider_ = new QSlider(Qt::Horizontal);
    smoothnessSlider_->setToolTip("Blends point influence regions (0=sharp, 1=smooth)");
    smoothnessSlider_->setRange(0, 100);
    smoothnessSlider_->setValue(50);
    smoothnessSlider_->setStyleSheet(sliderStyle);
    smoothLayout->addWidget(smoothnessSlider_);
    smoothnessValueLabel_ = new QLabel("0.50");
    smoothnessValueLabel_->setMinimumWidth(35);
    smoothnessValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    smoothLayout->addWidget(smoothnessValueLabel_);
    rbfLayout->addLayout(smoothLayout);

    // RBF Kernel type dropdown
    QHBoxLayout* kernelLayout = new QHBoxLayout();
    QLabel* kernelLabel = new QLabel("Kernel:");
    kernelLabel->setMinimumWidth(75);
    kernelLayout->addWidget(kernelLabel);
    kernelTypeCombo_ = new QComboBox();
    kernelTypeCombo_->setToolTip("RBF interpolation kernel");
    kernelTypeCombo_->addItem("Thin-Plate Spline", static_cast<int>(DeformationAlgorithm::RBF_TPS));
    kernelTypeCombo_->addItem("Gaussian", static_cast<int>(DeformationAlgorithm::RBF_GAUSSIAN));
    kernelTypeCombo_->addItem("Multiquadric", static_cast<int>(DeformationAlgorithm::RBF_MULTIQUADRIC));
    kernelTypeCombo_->setCurrentIndex(0);
    kernelTypeCombo_->setStyleSheet(comboStyle);
    kernelLayout->addWidget(kernelTypeCombo_);
    rbfLayout->addLayout(kernelLayout);

    connect(stiffnessSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnStiffnessChanged);
    connect(smoothnessSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnSmoothnessChanged);
    connect(kernelTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SidebarWidget::OnKernelTypeChanged);

    rbfOuterLayout->addWidget(rbfGroup);
    rbfParamsWidget_->setVisible(false); // Hidden by default (NRICP is default)
    controlsLayout->addWidget(rbfParamsWidget_);

    // --- Processing Group ---
    QGroupBox* processGroup = new QGroupBox("Processing");
    processGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* processLayout = new QVBoxLayout(processGroup);

    processButton_ = new QPushButton("Process Morph");
    processButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498DB;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    font-size: 11pt;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980B9;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #555;"
        "}"
    );
    processButton_->setToolTip("Run deformation to morph the mesh");
    connect(processButton_, &QPushButton::clicked, this, &SidebarWidget::ProcessMorphRequested);
    processLayout->addWidget(processButton_);

    cancelButton_ = new QPushButton("Cancel");
    cancelButton_->setStyleSheet(
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
    cancelButton_->setVisible(false);
    connect(cancelButton_, &QPushButton::clicked, this, &SidebarWidget::CancelMorphRequested);
    processLayout->addWidget(cancelButton_);

    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setVisible(false);
    progressBar_->setStyleSheet(
        "QProgressBar {"
        "    background-color: #34495E;"
        "    border: 1px solid #555;"
        "    border-radius: 4px;"
        "    text-align: center;"
        "    color: white;"
        "    height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #3498DB;"
        "    border-radius: 3px;"
        "}"
    );
    processLayout->addWidget(progressBar_);

    progressLabel_ = new QLabel("Ready to process");
    progressLabel_->setAlignment(Qt::AlignCenter);
    progressLabel_->setStyleSheet("QLabel { color: #95A5A6; font-size: 9pt; }");
    processLayout->addWidget(progressLabel_);

    controlsLayout->addWidget(processGroup);

    // --- Preview Group ---
    QGroupBox* previewGroup = new QGroupBox("Preview Mode");
    previewGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    previewModeCombo_ = new QComboBox();
    previewModeCombo_->setToolTip("Switch between different preview visualizations");
    previewModeCombo_->addItem("Deformed", static_cast<int>(MorphPreviewMode::Deformed));
    previewModeCombo_->addItem("Original", static_cast<int>(MorphPreviewMode::Original));
    previewModeCombo_->addItem("Overlay", static_cast<int>(MorphPreviewMode::Overlay));
    previewModeCombo_->addItem("Heat Map", static_cast<int>(MorphPreviewMode::HeatMap));
    previewModeCombo_->setCurrentIndex(0);
    previewModeCombo_->setEnabled(false);
    previewModeCombo_->setStyleSheet(comboStyle);
    connect(previewModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SidebarWidget::OnPreviewModeChanged);
    previewLayout->addWidget(previewModeCombo_);

    controlsLayout->addWidget(previewGroup);

    // --- Actions Group ---
    QGroupBox* actionsGroup = new QGroupBox("Actions");
    actionsGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup);

    acceptButton_ = new QPushButton("Accept Result");
    acceptButton_->setEnabled(false);
    acceptButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #2ECC71;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    font-size: 10pt;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #27AE60;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #555;"
        "}"
    );
    acceptButton_->setToolTip("Accept the morph result and proceed to Touch Up");
    connect(acceptButton_, &QPushButton::clicked, this, &SidebarWidget::AcceptMorphRequested);
    actionsLayout->addWidget(acceptButton_);

    reprocessButton_ = new QPushButton("Re-process");
    reprocessButton_->setEnabled(false);
    reprocessButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #F39C12;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    font-size: 10pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #E67E22;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #555;"
        "}"
    );
    connect(reprocessButton_, &QPushButton::clicked, this, &SidebarWidget::ProcessMorphRequested);
    actionsLayout->addWidget(reprocessButton_);

    resetDefaultsButton_ = new QPushButton("Reset to Defaults");
    resetDefaultsButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #7F8C8D;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    font-size: 9pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #95A5A6;"
        "}"
    );
    connect(resetDefaultsButton_, &QPushButton::clicked, this, [this]() {
        // Reset based on currently visible algorithm panel
        if (nricpParamsWidget_ && nricpParamsWidget_->isVisible()) {
            if (nricpAlphaInitialSlider_) nricpAlphaInitialSlider_->setValue(100);
            if (nricpAlphaFinalSlider_) nricpAlphaFinalSlider_->setValue(50);
            if (nricpStiffnessStepsSlider_) nricpStiffnessStepsSlider_->setValue(5);
            if (nricpIcpIterationsSlider_) nricpIcpIterationsSlider_->setValue(7);
            if (nricpNormalThresholdSlider_) nricpNormalThresholdSlider_->setValue(60);
            if (nricpLandmarkWeightSlider_) nricpLandmarkWeightSlider_->setValue(10);
            if (nricpBoundaryExclusionCheckBox_) nricpBoundaryExclusionCheckBox_->setChecked(true);
            if (nricpBoundaryHopsSlider_) nricpBoundaryHopsSlider_->setValue(3);
            if (nricpOptimizationIterationsSlider_) nricpOptimizationIterationsSlider_->setValue(1);
            if (nricpDpInitialSlider_) nricpDpInitialSlider_->setValue(100);
            if (nricpDpFinalSlider_) nricpDpFinalSlider_->setValue(100);
            if (nricpGammaInitialSlider_) nricpGammaInitialSlider_->setValue(0);
            if (nricpGammaFinalSlider_) nricpGammaFinalSlider_->setValue(0);
            if (nricpSamplingInitialSlider_) nricpSamplingInitialSlider_->setValue(100);
            if (nricpSamplingFinalSlider_) nricpSamplingFinalSlider_->setValue(10);
            if (nricpNormalizeSamplingCheckBox_) nricpNormalizeSamplingCheckBox_->setChecked(true);
        } else {
            if (stiffnessSlider_) stiffnessSlider_->setValue(50);
            if (smoothnessSlider_) smoothnessSlider_->setValue(50);
            if (kernelTypeCombo_) kernelTypeCombo_->setCurrentIndex(0);
        }
    });
    actionsLayout->addWidget(resetDefaultsButton_);

    controlsLayout->addWidget(actionsGroup);
    controlsLayout->addStretch();

    nextStageButton_->setEnabled(false);
}

void SidebarWidget::CreateTouchUpControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(16);

    QLabel* info = new QLabel("Refine the mesh with sculpting tools.\nLeft-click and drag on the mesh to sculpt.");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);

    // Common styles
    QString groupBoxStyle =
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }";
    QString comboStyle =
        "QComboBox { background-color: #34495E; color: white; border: 1px solid #555; padding: 4px; border-radius: 3px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #34495E; color: white; selection-background-color: #3498DB; }";
    QString buttonStyle =
        "QPushButton { background-color: #34495E; padding: 8px 12px; border: 2px solid #555; border-radius: 4px; }"
        "QPushButton:checked { background-color: #3498DB; border-color: #3498DB; }";

    // Brush Tool Selector
    QGroupBox* toolGroup = new QGroupBox("Brush Tools");
    toolGroup->setStyleSheet(groupBoxStyle);
    QGridLayout* toolLayout = new QGridLayout(toolGroup);

    brushButtonGroup_ = new QButtonGroup(this);
    brushButtonGroup_->setExclusive(true);

    smoothBrushButton_ = new QPushButton("Smooth");
    smoothBrushButton_->setToolTip("Average vertex positions to smooth the surface");
    smoothBrushButton_->setCheckable(true);
    smoothBrushButton_->setChecked(true);
    smoothBrushButton_->setStyleSheet(buttonStyle);
    brushButtonGroup_->addButton(smoothBrushButton_, 0);
    toolLayout->addWidget(smoothBrushButton_, 0, 0);

    grabBrushButton_ = new QPushButton("Grab");
    grabBrushButton_->setToolTip("Click and drag to move vertices with falloff");
    grabBrushButton_->setCheckable(true);
    grabBrushButton_->setStyleSheet(buttonStyle);
    brushButtonGroup_->addButton(grabBrushButton_, 1);
    toolLayout->addWidget(grabBrushButton_, 0, 1);

    pushPullBrushButton_ = new QPushButton("Push/Pull");
    pushPullBrushButton_->setToolTip("Push vertices inward or pull outward along surface normal");
    pushPullBrushButton_->setCheckable(true);
    pushPullBrushButton_->setStyleSheet(buttonStyle);
    brushButtonGroup_->addButton(pushPullBrushButton_, 2);
    toolLayout->addWidget(pushPullBrushButton_, 1, 0);

    inflateBrushButton_ = new QPushButton("Inflate");
    inflateBrushButton_->setToolTip("Expand or contract the surface along vertex normals");
    inflateBrushButton_->setCheckable(true);
    inflateBrushButton_->setStyleSheet(buttonStyle);
    brushButtonGroup_->addButton(inflateBrushButton_, 3);
    toolLayout->addWidget(inflateBrushButton_, 1, 1);

    connect(brushButtonGroup_, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
        BrushType type;
        switch (id) {
            case 0: type = BrushType::Smooth; break;
            case 1: type = BrushType::Grab; break;
            case 2: type = BrushType::PushPull; break;
            case 3: type = BrushType::Inflate; break;
            default: type = BrushType::Smooth; break;
        }
        emit BrushTypeChanged(type);
    });

    controlsLayout->addWidget(toolGroup);

    // Brush Settings
    QGroupBox* settingsGroup = new QGroupBox("Brush Settings");
    settingsGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsGroup);
    settingsLayout->setSpacing(8);

    // Radius slider
    QHBoxLayout* radiusLabelLayout = new QHBoxLayout();
    radiusLabelLayout->addWidget(new QLabel("Radius:"));
    brushRadiusValueLabel_ = new QLabel("0.5");
    brushRadiusValueLabel_->setAlignment(Qt::AlignRight);
    radiusLabelLayout->addWidget(brushRadiusValueLabel_);
    settingsLayout->addLayout(radiusLabelLayout);

    brushRadiusSlider_ = new QSlider(Qt::Horizontal);
    brushRadiusSlider_->setToolTip("Brush radius - use [ ] keys to adjust");
    brushRadiusSlider_->setRange(1, 1000);
    brushRadiusSlider_->setValue(5);
    connect(brushRadiusSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnBrushRadiusChanged);
    settingsLayout->addWidget(brushRadiusSlider_);

    QLabel* radiusHint = new QLabel("[ ] keys to adjust");
    radiusHint->setStyleSheet("QLabel { color: #888; font-size: 10px; }");
    settingsLayout->addWidget(radiusHint);

    // Strength slider
    QHBoxLayout* strengthLabelLayout = new QHBoxLayout();
    strengthLabelLayout->addWidget(new QLabel("Strength:"));
    brushStrengthValueLabel_ = new QLabel("0.50");
    brushStrengthValueLabel_->setAlignment(Qt::AlignRight);
    strengthLabelLayout->addWidget(brushStrengthValueLabel_);
    settingsLayout->addLayout(strengthLabelLayout);

    brushStrengthSlider_ = new QSlider(Qt::Horizontal);
    brushStrengthSlider_->setToolTip("Brush effect intensity");
    brushStrengthSlider_->setRange(1, 100);
    brushStrengthSlider_->setValue(50);
    connect(brushStrengthSlider_, &QSlider::valueChanged, this, &SidebarWidget::OnBrushStrengthChanged);
    settingsLayout->addWidget(brushStrengthSlider_);

    // Falloff type
    QHBoxLayout* falloffLayout = new QHBoxLayout();
    falloffLayout->addWidget(new QLabel("Falloff:"));
    falloffTypeCombo_ = new QComboBox();
    falloffTypeCombo_->setToolTip("How brush strength decreases from center to edge");
    falloffTypeCombo_->addItem("Smooth");
    falloffTypeCombo_->addItem("Linear");
    falloffTypeCombo_->addItem("Sharp");
    falloffTypeCombo_->setCurrentIndex(0);
    falloffTypeCombo_->setStyleSheet(comboStyle);
    connect(falloffTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SidebarWidget::OnFalloffTypeChanged);
    falloffLayout->addWidget(falloffTypeCombo_);
    settingsLayout->addLayout(falloffLayout);

    controlsLayout->addWidget(settingsGroup);

    // Sculpting Symmetry
    QGroupBox* symmetryGroup = new QGroupBox("Sculpting Symmetry");
    symmetryGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* symmetryLayout = new QVBoxLayout(symmetryGroup);

    sculptSymmetryCheckBox_ = new QCheckBox("Enable Symmetry");
    sculptSymmetryCheckBox_->setToolTip("Mirror sculpting strokes across the symmetry axis");
    sculptSymmetryCheckBox_->setStyleSheet("QCheckBox { color: white; }");
    symmetryLayout->addWidget(sculptSymmetryCheckBox_);

    QHBoxLayout* symAxisLayout = new QHBoxLayout();
    QLabel* symAxisLabel = new QLabel("Axis:");
    symAxisLabel->setStyleSheet("QLabel { color: #BDC3C7; }");
    symAxisLayout->addWidget(symAxisLabel);
    sculptSymmetryAxisCombo_ = new QComboBox();
    sculptSymmetryAxisCombo_->addItem("X", static_cast<int>(Axis::X));
    sculptSymmetryAxisCombo_->addItem("Y", static_cast<int>(Axis::Y));
    sculptSymmetryAxisCombo_->addItem("Z", static_cast<int>(Axis::Z));
    sculptSymmetryAxisCombo_->setCurrentIndex(0);
    sculptSymmetryAxisCombo_->setEnabled(false);
    sculptSymmetryAxisCombo_->setStyleSheet(comboStyle);
    symAxisLayout->addWidget(sculptSymmetryAxisCombo_);
    symAxisLayout->addStretch();
    symmetryLayout->addLayout(symAxisLayout);

    connect(sculptSymmetryCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (sculptSymmetryAxisCombo_) sculptSymmetryAxisCombo_->setEnabled(checked);
        Axis axis = static_cast<Axis>(sculptSymmetryAxisCombo_->currentData().toInt());
        emit SculptSymmetryChanged(checked, axis);
    });
    connect(sculptSymmetryAxisCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (sculptSymmetryCheckBox_ && sculptSymmetryCheckBox_->isChecked()) {
            Axis axis = static_cast<Axis>(sculptSymmetryAxisCombo_->currentData().toInt());
            emit SculptSymmetryChanged(true, axis);
        }
    });

    controlsLayout->addWidget(symmetryGroup);

    // Display Options
    QGroupBox* displayGroup = new QGroupBox("Display Options");
    displayGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

    showOverlayCheckBox_ = new QCheckBox("Show Target Overlay");
    showOverlayCheckBox_->setStyleSheet("QCheckBox { color: white; }");
    connect(showOverlayCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        emit ShowTargetOverlayChanged(checked);
    });
    displayLayout->addWidget(showOverlayCheckBox_);

    controlsLayout->addWidget(displayGroup);

    controlsLayout->addStretch();

    // Finalize button
    finalizeButton_ = new QPushButton("Finalize and Export");
    finalizeButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #2ECC71;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    font-size: 12pt;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #27AE60;"
        "}"
    );
    finalizeButton_->setToolTip("Export the finished mesh for use in Unreal Engine");
    connect(finalizeButton_, &QPushButton::clicked, this, &SidebarWidget::FinalizeRequested);
    controlsLayout->addWidget(finalizeButton_);

    // Hide next stage button for final stage
    nextStageButton_->setVisible(false);
}

void SidebarWidget::OnBrushRadiusChanged(int value) {
    float radius = value / 10.0f;
    if (brushRadiusValueLabel_) {
        brushRadiusValueLabel_->setText(QString::number(radius, 'f', 1));
    }
    emit BrushRadiusChangedSignal(radius);
}

void SidebarWidget::OnBrushStrengthChanged(int value) {
    float strength = value / 100.0f;
    if (brushStrengthValueLabel_) {
        brushStrengthValueLabel_->setText(QString::number(strength, 'f', 2));
    }
    emit BrushStrengthChangedSignal(strength);
}

void SidebarWidget::OnFalloffTypeChanged(int index) {
    FalloffType falloff;
    switch (index) {
        case 0: falloff = FalloffType::Smooth; break;
        case 1: falloff = FalloffType::Linear; break;
        case 2: falloff = FalloffType::Sharp; break;
        default: falloff = FalloffType::Smooth; break;
    }
    emit BrushFalloffChanged(falloff);
}

void SidebarWidget::SetBrushRadius(float radius) {
    if (brushRadiusSlider_) {
        brushRadiusSlider_->blockSignals(true);
        brushRadiusSlider_->setValue(static_cast<int>(radius * 10.0f));
        brushRadiusSlider_->blockSignals(false);
    }
    if (brushRadiusValueLabel_) {
        brushRadiusValueLabel_->setText(QString::number(radius, 'f', 1));
    }
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
    algorithmCombo_ = nullptr;
    rbfParamsWidget_ = nullptr; nricpParamsWidget_ = nullptr;
    stiffnessSlider_ = nullptr; smoothnessSlider_ = nullptr;
    stiffnessValueLabel_ = nullptr; smoothnessValueLabel_ = nullptr;
    kernelTypeCombo_ = nullptr;
    nricpAlphaInitialSlider_ = nullptr; nricpAlphaFinalSlider_ = nullptr;
    nricpStiffnessStepsSlider_ = nullptr; nricpIcpIterationsSlider_ = nullptr;
    nricpNormalThresholdSlider_ = nullptr; nricpLandmarkWeightSlider_ = nullptr;
    nricpAlphaInitialLabel_ = nullptr; nricpAlphaFinalLabel_ = nullptr;
    nricpStiffnessStepsLabel_ = nullptr; nricpIcpIterationsLabel_ = nullptr;
    nricpNormalThresholdLabel_ = nullptr; nricpLandmarkWeightLabel_ = nullptr;
    nricpBoundaryExclusionCheckBox_ = nullptr;
    nricpBoundaryHopsSlider_ = nullptr; nricpBoundaryHopsLabel_ = nullptr;
    processButton_ = nullptr; cancelButton_ = nullptr;
    progressBar_ = nullptr; progressLabel_ = nullptr; previewModeCombo_ = nullptr;
    acceptButton_ = nullptr; reprocessButton_ = nullptr; resetDefaultsButton_ = nullptr;
    brushButtonGroup_ = nullptr; smoothBrushButton_ = nullptr; grabBrushButton_ = nullptr;
    pushPullBrushButton_ = nullptr; inflateBrushButton_ = nullptr;
    brushRadiusSlider_ = nullptr; brushRadiusValueLabel_ = nullptr;
    brushStrengthSlider_ = nullptr; brushStrengthValueLabel_ = nullptr;
    falloffTypeCombo_ = nullptr;
    sculptSymmetryCheckBox_ = nullptr; sculptSymmetryAxisCombo_ = nullptr;
    showOverlayCheckBox_ = nullptr; finalizeButton_ = nullptr;
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

void SidebarWidget::OnStiffnessChanged(int value) {
    if (stiffnessValueLabel_) {
        stiffnessValueLabel_->setText(QString::number(value / 100.0f, 'f', 2));
    }
    if (project_) {
        project_->GetMorphData().stiffness = value / 100.0f;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnSmoothnessChanged(int value) {
    if (smoothnessValueLabel_) {
        smoothnessValueLabel_->setText(QString::number(value / 100.0f, 'f', 2));
    }
    if (project_) {
        project_->GetMorphData().smoothness = value / 100.0f;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnKernelTypeChanged(int index) {
    Q_UNUSED(index);
    if (project_ && kernelTypeCombo_) {
        project_->GetMorphData().algorithm =
            static_cast<DeformationAlgorithm>(kernelTypeCombo_->currentData().toInt());
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnPreviewModeChanged(int index) {
    if (!previewModeCombo_) return;
    MorphPreviewMode mode = static_cast<MorphPreviewMode>(previewModeCombo_->itemData(index).toInt());
    emit MorphPreviewModeChanged(mode);
}

void SidebarWidget::OnMorphProgress(float progress, const QString& message) {
    if (progressBar_) {
        progressBar_->setValue(static_cast<int>(progress * 100));
    }
    if (progressLabel_) {
        progressLabel_->setText(message);
        progressLabel_->setStyleSheet("QLabel { color: #3498DB; font-size: 9pt; }");
    }
}

void SidebarWidget::OnMorphComplete(bool success, const QString& message) {
    SetMorphProcessing(false);

    if (progressLabel_) {
        progressLabel_->setText(message);
        if (success) {
            progressLabel_->setStyleSheet("QLabel { color: #2ECC71; font-size: 9pt; font-weight: bold; }");
        } else {
            progressLabel_->setStyleSheet("QLabel { color: #E74C3C; font-size: 9pt; font-weight: bold; }");
        }
    }

    if (success) {
        if (previewModeCombo_) previewModeCombo_->setEnabled(true);
        if (acceptButton_) acceptButton_->setEnabled(true);
        if (reprocessButton_) reprocessButton_->setEnabled(true);
    }
}

void SidebarWidget::SetMorphProcessing(bool processing) {
    if (processButton_) processButton_->setEnabled(!processing);
    if (cancelButton_) cancelButton_->setVisible(processing);
    if (progressBar_) {
        progressBar_->setVisible(processing);
        if (processing) progressBar_->setValue(0);
    }
    if (algorithmCombo_) algorithmCombo_->setEnabled(!processing);
    // RBF controls
    if (stiffnessSlider_) stiffnessSlider_->setEnabled(!processing);
    if (smoothnessSlider_) smoothnessSlider_->setEnabled(!processing);
    if (kernelTypeCombo_) kernelTypeCombo_->setEnabled(!processing);
    // NRICP controls
    if (nricpAlphaInitialSlider_) nricpAlphaInitialSlider_->setEnabled(!processing);
    if (nricpAlphaFinalSlider_) nricpAlphaFinalSlider_->setEnabled(!processing);
    if (nricpStiffnessStepsSlider_) nricpStiffnessStepsSlider_->setEnabled(!processing);
    if (nricpIcpIterationsSlider_) nricpIcpIterationsSlider_->setEnabled(!processing);
    if (nricpNormalThresholdSlider_) nricpNormalThresholdSlider_->setEnabled(!processing);
    if (nricpLandmarkWeightSlider_) nricpLandmarkWeightSlider_->setEnabled(!processing);
    if (nricpBoundaryExclusionCheckBox_) nricpBoundaryExclusionCheckBox_->setEnabled(!processing);
    if (nricpBoundaryHopsSlider_) nricpBoundaryHopsSlider_->setEnabled(!processing &&
        (nricpBoundaryExclusionCheckBox_ ? nricpBoundaryExclusionCheckBox_->isChecked() : true));
    if (nricpOptimizationIterationsSlider_) nricpOptimizationIterationsSlider_->setEnabled(!processing);
    if (nricpDpInitialSlider_) nricpDpInitialSlider_->setEnabled(!processing);
    if (nricpDpFinalSlider_) nricpDpFinalSlider_->setEnabled(!processing);
    if (nricpGammaInitialSlider_) nricpGammaInitialSlider_->setEnabled(!processing);
    if (nricpGammaFinalSlider_) nricpGammaFinalSlider_->setEnabled(!processing);
    if (nricpSamplingInitialSlider_) nricpSamplingInitialSlider_->setEnabled(!processing);
    if (nricpSamplingFinalSlider_) nricpSamplingFinalSlider_->setEnabled(!processing);
    if (nricpNormalizeSamplingCheckBox_) nricpNormalizeSamplingCheckBox_->setEnabled(!processing);
    if (resetDefaultsButton_) resetDefaultsButton_->setEnabled(!processing);
    if (acceptButton_) acceptButton_->setEnabled(false);
    if (reprocessButton_) reprocessButton_->setEnabled(false);

    if (processing && progressLabel_) {
        progressLabel_->setText("Processing...");
        progressLabel_->setStyleSheet("QLabel { color: #3498DB; font-size: 9pt; }");
    }
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

void SidebarWidget::OnAlgorithmChanged(int index) {
    if (!algorithmCombo_) return;

    DeformationAlgorithm algo = static_cast<DeformationAlgorithm>(algorithmCombo_->itemData(index).toInt());
    bool isNRICP = (algo == DeformationAlgorithm::NRICP);

    if (nricpParamsWidget_) nricpParamsWidget_->setVisible(isNRICP);
    if (rbfParamsWidget_) rbfParamsWidget_->setVisible(!isNRICP);

    if (project_) {
        project_->GetMorphData().algorithm = algo;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPAlphaInitialChanged(int value) {
    if (nricpAlphaInitialLabel_) {
        nricpAlphaInitialLabel_->setText(QString::number(value));
    }
    if (project_) {
        project_->GetMorphData().nricpAlphaInitial = static_cast<float>(value);
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPAlphaFinalChanged(int value) {
    if (nricpAlphaFinalLabel_) {
        nricpAlphaFinalLabel_->setText(QString::number(value));
    }
    if (project_) {
        project_->GetMorphData().nricpAlphaFinal = static_cast<float>(value);
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPStiffnessStepsChanged(int value) {
    if (nricpStiffnessStepsLabel_) {
        nricpStiffnessStepsLabel_->setText(QString::number(value));
    }
    if (project_) {
        project_->GetMorphData().nricpStiffnessSteps = value;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPIcpIterationsChanged(int value) {
    if (nricpIcpIterationsLabel_) {
        nricpIcpIterationsLabel_->setText(QString::number(value));
    }
    if (project_) {
        project_->GetMorphData().nricpIcpIterations = value;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPNormalThresholdChanged(int value) {
    if (nricpNormalThresholdLabel_) {
        nricpNormalThresholdLabel_->setText(QString::number(value) + QString::fromUtf8("\u00B0"));
    }
    if (project_) {
        project_->GetMorphData().nricpNormalThreshold = static_cast<float>(value);
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPLandmarkWeightChanged(int value) {
    if (nricpLandmarkWeightLabel_) {
        nricpLandmarkWeightLabel_->setText(QString::number(static_cast<float>(value), 'f', 1));
    }
    if (project_) {
        project_->GetMorphData().nricpLandmarkWeight = static_cast<float>(value);
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPBoundaryExclusionToggled(bool enabled) {
    if (nricpBoundaryHopsSlider_) {
        nricpBoundaryHopsSlider_->setEnabled(enabled);
    }
    if (project_) {
        project_->GetMorphData().nricpEnableBoundaryExclusion = enabled;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPBoundaryHopsChanged(int value) {
    if (nricpBoundaryHopsLabel_) {
        nricpBoundaryHopsLabel_->setText(QString::number(value));
    }
    if (project_) {
        project_->GetMorphData().nricpBoundaryExclusionHops = value;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPOptimizationIterationsChanged(int value) {
    if (nricpOptimizationIterationsLabel_) {
        nricpOptimizationIterationsLabel_->setText(QString::number(value));
    }
    if (project_) {
        project_->GetMorphData().nricpOptimizationIterations = value;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPDpInitialChanged(int value) {
    float dpVal = static_cast<float>(value) / 100.0f;
    if (nricpDpInitialLabel_) {
        nricpDpInitialLabel_->setText(QString::number(dpVal, 'f', 2));
    }
    if (project_) {
        project_->GetMorphData().nricpDpInitial = dpVal;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPDpFinalChanged(int value) {
    float dpVal = static_cast<float>(value) / 100.0f;
    if (nricpDpFinalLabel_) {
        nricpDpFinalLabel_->setText(QString::number(dpVal, 'f', 2));
    }
    if (project_) {
        project_->GetMorphData().nricpDpFinal = dpVal;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPGammaInitialChanged(int value) {
    float gammaVal = static_cast<float>(value) / 10.0f;
    if (nricpGammaInitialLabel_) {
        nricpGammaInitialLabel_->setText(QString::number(gammaVal, 'f', 1));
    }
    if (project_) {
        project_->GetMorphData().nricpGammaInitial = gammaVal;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPGammaFinalChanged(int value) {
    float gammaVal = static_cast<float>(value) / 10.0f;
    if (nricpGammaFinalLabel_) {
        nricpGammaFinalLabel_->setText(QString::number(gammaVal, 'f', 1));
    }
    if (project_) {
        project_->GetMorphData().nricpGammaFinal = gammaVal;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPSamplingInitialChanged(int value) {
    float sampVal = static_cast<float>(value) / 1000.0f;
    if (nricpSamplingInitialLabel_) {
        nricpSamplingInitialLabel_->setText(value == 0 ? "All" : QString::number(sampVal, 'f', 3));
    }
    if (project_) {
        project_->GetMorphData().nricpSamplingInitial = sampVal;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPSamplingFinalChanged(int value) {
    float sampVal = static_cast<float>(value) / 1000.0f;
    if (nricpSamplingFinalLabel_) {
        nricpSamplingFinalLabel_->setText(value == 0 ? "All" : QString::number(sampVal, 'f', 3));
    }
    if (project_) {
        project_->GetMorphData().nricpSamplingFinal = sampVal;
    }
    emit MorphParameterChanged();
}

void SidebarWidget::OnNRICPNormalizeSamplingToggled(bool enabled) {
    if (project_) {
        project_->GetMorphData().nricpNormalizeSampling = enabled;
    }
    emit MorphParameterChanged();
}

} // namespace MetaVisage
