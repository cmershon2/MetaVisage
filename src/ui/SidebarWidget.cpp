#include "ui/SidebarWidget.h"
#include <QGroupBox>
#include <QPushButton>
#include <QSpacerItem>

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

    QLabel* info = new QLabel("Load meshes and align them.");
    info->setWordWrap(true);
    controlsLayout->addWidget(info);

    // TODO: Add alignment controls in Sprint 3
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
