#include "ui/SidebarWidget.h"
#include <QGroupBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>

namespace MetaVisage {

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent),
      layout_(nullptr),
      stageLabel_(nullptr),
      nextStageButton_(nullptr),
      controlsWidget_(nullptr) {

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

    QLabel* toolsInfo = new QLabel("Use G (Move), R (Rotate), S (Scale) keys\nTransform gizmos coming in next update");
    toolsInfo->setWordWrap(true);
    toolsInfo->setStyleSheet("QLabel { color: #95A5A6; font-size: 9pt; }");
    toolsLayout->addWidget(toolsInfo);

    controlsLayout->addWidget(toolsGroup);

    controlsLayout->addStretch();
}

void SidebarWidget::CreatePointReferenceControls() {
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget_);
    controlsLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* info = new QLabel("Place correspondence points on both meshes.");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);

    // TODO: Add point reference controls in Sprint 4-5
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
    if (controlsWidget_->layout()) {
        QLayoutItem* item;
        while ((item = controlsWidget_->layout()->takeAt(0))) {
            delete item->widget();
            delete item;
        }
        delete controlsWidget_->layout();
    }
}

} // namespace MetaVisage
