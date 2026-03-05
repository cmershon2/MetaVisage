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
      clearAllPointsButton_(nullptr),
      updatingTransformDisplay_(false) {

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

    // Helper lambda to create position spinbox
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

    // Helper lambda to create rotation spinbox (degrees)
    auto createRotSpinBox = [&]() -> QDoubleSpinBox* {
        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        spinBox->setRange(-360.0, 360.0);
        spinBox->setDecimals(1);
        spinBox->setSingleStep(1.0);
        spinBox->setValue(0.0);
        spinBox->setSuffix(QString::fromUtf8("\u00B0"));  // Degree symbol
        spinBox->setStyleSheet(spinBoxStyle);
        spinBox->setMinimumWidth(55);
        return spinBox;
    };

    // Helper lambda to create scale spinbox
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

    // Update initial display
    UpdateTransformDisplay();
}

void SidebarWidget::CreatePointReferenceControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(16);

    // Instructions
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
    targetPointCountLabel_->setObjectName("targetPointCount");
    targetPointCountLabel_->setStyleSheet("QLabel { color: #F39C12; font-size: 10pt; }"); // Orange for target
    countLayout->addWidget(targetPointCountLabel_);

    morphPointCountLabel_ = new QLabel("Morph Points: 0");
    morphPointCountLabel_->setObjectName("morphPointCount");
    morphPointCountLabel_->setStyleSheet("QLabel { color: #3498DB; font-size: 10pt; }"); // Blue for morph
    countLayout->addWidget(morphPointCountLabel_);

    // Match status indicator
    matchStatusLabel_ = new QLabel("No points placed");
    matchStatusLabel_->setObjectName("matchStatus");
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

    // Scrollable point list
    pointListScroll_ = new QScrollArea();
    pointListScroll_->setWidgetResizable(true);
    pointListScroll_->setMinimumHeight(150);
    pointListScroll_->setMaximumHeight(300);
    pointListScroll_->setStyleSheet(
        "QScrollArea { background-color: #34495E; border: 1px solid #555; border-radius: 3px; }"
        "QScrollBar:vertical { background-color: #2C3E50; width: 10px; }"
        "QScrollBar::handle:vertical { background-color: #555; border-radius: 5px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );

    QWidget* pointListContent = new QWidget();
    QVBoxLayout* pointListLayout = new QVBoxLayout(pointListContent);
    pointListLayout->setContentsMargins(8, 8, 8, 8);

    QLabel* noPointsLabel = new QLabel("No points placed yet.\n\nPoint placement will be\navailable in Sprint 5.");
    noPointsLabel->setAlignment(Qt::AlignCenter);
    noPointsLabel->setStyleSheet("QLabel { color: #7F8C8D; font-style: italic; }");
    pointListLayout->addWidget(noPointsLabel);
    pointListLayout->addStretch();

    pointListScroll_->setWidget(pointListContent);
    listLayout->addWidget(pointListScroll_);

    controlsLayout->addWidget(listGroup);

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

    controlsLayout->addWidget(actionsGroup);

    // Symmetry Group (scaffolding for Sprint 5)
    QGroupBox* symmetryGroup = new QGroupBox("Symmetry");
    symmetryGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
    );
    QVBoxLayout* symmetryLayout = new QVBoxLayout(symmetryGroup);

    QCheckBox* symmetryCheck = new QCheckBox("Enable Symmetry Mode");
    symmetryCheck->setEnabled(false);
    symmetryCheck->setStyleSheet("QCheckBox { color: #7F8C8D; }");
    symmetryLayout->addWidget(symmetryCheck);

    QLabel* symmetryNote = new QLabel("Symmetry mode will be available in Sprint 5");
    symmetryNote->setStyleSheet("QLabel { color: #7F8C8D; font-size: 8pt; font-style: italic; }");
    symmetryNote->setWordWrap(true);
    symmetryLayout->addWidget(symmetryNote);

    controlsLayout->addWidget(symmetryGroup);

    controlsLayout->addStretch();

    // Disable next stage by default (needs matching point counts)
    nextStageButton_->setEnabled(false);
}

void SidebarWidget::CreateMorphControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* info = new QLabel("Adjust parameters and process the morph.");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);

    // TODO: Add morph controls in Sprint 6-7
}

void SidebarWidget::CreateTouchUpControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* info = new QLabel("Refine the mesh with sculpting tools.");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);

    // TODO: Add sculpting controls in Sprint 8-9
}

void SidebarWidget::ClearControls() {
    // Reset pointers before clearing (they will be deleted with their widgets)
    transformModeLabel_ = nullptr;
    posXSpinBox_ = nullptr;
    posYSpinBox_ = nullptr;
    posZSpinBox_ = nullptr;
    rotXSpinBox_ = nullptr;
    rotYSpinBox_ = nullptr;
    rotZSpinBox_ = nullptr;
    scaleXSpinBox_ = nullptr;
    scaleYSpinBox_ = nullptr;
    scaleZSpinBox_ = nullptr;
    resetTransformButton_ = nullptr;
    targetPointCountLabel_ = nullptr;
    morphPointCountLabel_ = nullptr;
    matchStatusLabel_ = nullptr;
    pointListScroll_ = nullptr;
    clearAllPointsButton_ = nullptr;

    if (controlsWidget_->layout()) {
        QLayoutItem* item;
        while ((item = controlsWidget_->layout()->takeAt(0))) {
            delete item->widget();
            delete item;
        }
        delete controlsWidget_->layout();
    }
}

void SidebarWidget::OnTransformModeChanged(TransformMode mode, AxisConstraint axis) {
    if (!transformModeLabel_) return;

    QString modeStr;
    switch (mode) {
        case TransformMode::Move:
            modeStr = "Move";
            break;
        case TransformMode::Rotate:
            modeStr = "Rotate";
            break;
        case TransformMode::Scale:
            modeStr = "Scale";
            break;
        case TransformMode::None:
        default:
            modeStr = "None";
            break;
    }

    QString axisStr;
    switch (axis) {
        case AxisConstraint::X:
            axisStr = " [X only]";
            break;
        case AxisConstraint::Y:
            axisStr = " [Y only]";
            break;
        case AxisConstraint::Z:
            axisStr = " [Z only]";
            break;
        case AxisConstraint::None:
        default:
            axisStr = "";
            break;
    }

    transformModeLabel_->setText("Mode: " + modeStr + axisStr);

    // Highlight active mode with different colors
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
        !scaleXSpinBox_ || !scaleYSpinBox_ || !scaleZSpinBox_) {
        return;
    }

    // Set flag to prevent feedback loop when updating spinboxes programmatically
    updatingTransformDisplay_ = true;

    const Transform& transform = project_->GetTargetMesh().transform;
    Vector3 pos = transform.GetPosition();
    Vector3 euler = transform.GetRotation().ToEulerAngles();
    Vector3 scale = transform.GetScale();

    posXSpinBox_->setValue(pos.x);
    posYSpinBox_->setValue(pos.y);
    posZSpinBox_->setValue(pos.z);

    rotXSpinBox_->setValue(euler.x);
    rotYSpinBox_->setValue(euler.y);
    rotZSpinBox_->setValue(euler.z);

    scaleXSpinBox_->setValue(scale.x);
    scaleYSpinBox_->setValue(scale.y);
    scaleZSpinBox_->setValue(scale.z);

    updatingTransformDisplay_ = false;
}

void SidebarWidget::OnSpinBoxValueChanged() {
    // Ignore changes while we're programmatically updating the display
    if (updatingTransformDisplay_) {
        return;
    }

    if (!project_ || !project_->GetTargetMesh().isLoaded) {
        return;
    }

    if (!posXSpinBox_ || !posYSpinBox_ || !posZSpinBox_ ||
        !rotXSpinBox_ || !rotYSpinBox_ || !rotZSpinBox_ ||
        !scaleXSpinBox_ || !scaleYSpinBox_ || !scaleZSpinBox_) {
        return;
    }

    // Get current spinbox values
    Vector3 newPos(
        static_cast<float>(posXSpinBox_->value()),
        static_cast<float>(posYSpinBox_->value()),
        static_cast<float>(posZSpinBox_->value())
    );

    Quaternion newRot = Quaternion::FromEulerAngles(
        static_cast<float>(rotXSpinBox_->value()),
        static_cast<float>(rotYSpinBox_->value()),
        static_cast<float>(rotZSpinBox_->value())
    );

    Vector3 newScale(
        static_cast<float>(scaleXSpinBox_->value()),
        static_cast<float>(scaleYSpinBox_->value()),
        static_cast<float>(scaleZSpinBox_->value())
    );

    // Apply to the target mesh transform
    Transform& transform = project_->GetTargetMesh().transform;
    transform.SetPosition(newPos);
    transform.SetRotation(newRot);
    transform.SetScale(newScale);

    // Signal that transform values changed so viewport can update
    emit TransformValuesChanged();
}

} // namespace MetaVisage
