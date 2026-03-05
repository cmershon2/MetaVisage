#include "ui/ViewportContainer.h"
#include "ui/ViewportWidget.h"
#include "core/Camera.h"
#include "core/Project.h"
#include <QHBoxLayout>
#include <QDebug>

namespace MetaVisage {

ViewportContainer::ViewportContainer(QWidget *parent)
    : QWidget(parent),
      splitter_(nullptr),
      leftViewport_(nullptr),
      rightViewport_(nullptr),
      dualMode_(false),
      syncing_(false) {

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->setHandleWidth(4);
    splitter_->setStyleSheet(
        "QSplitter::handle {"
        "    background-color: #34495E;"
        "}"
    );

    // Create left viewport (always visible)
    leftViewport_ = new ViewportWidget(splitter_);
    splitter_->addWidget(leftViewport_);

    // Create right viewport (hidden in single mode)
    rightViewport_ = new ViewportWidget(splitter_);
    splitter_->addWidget(rightViewport_);
    rightViewport_->hide();

    layout->addWidget(splitter_);

    // Connect camera sync signals
    connect(leftViewport_, &ViewportWidget::CameraChanged,
            this, &ViewportContainer::OnLeftCameraChanged);
    connect(rightViewport_, &ViewportWidget::CameraChanged,
            this, &ViewportContainer::OnRightCameraChanged);

    // Forward transform signals from left viewport (primary)
    connect(leftViewport_, &ViewportWidget::TransformModeChanged,
            this, &ViewportContainer::TransformModeChanged);
    connect(leftViewport_, &ViewportWidget::TargetTransformChanged,
            this, &ViewportContainer::TargetTransformChanged);

    // Forward point signals from both viewports
    connect(leftViewport_, &ViewportWidget::PointPlaced,
            this, &ViewportContainer::PointPlaced);
    connect(rightViewport_, &ViewportWidget::PointPlaced,
            this, &ViewportContainer::PointPlaced);
    connect(leftViewport_, &ViewportWidget::PointSelected,
            this, &ViewportContainer::PointSelected);
    connect(rightViewport_, &ViewportWidget::PointSelected,
            this, &ViewportContainer::PointSelected);
    connect(leftViewport_, &ViewportWidget::PointDeleteRequested,
            this, &ViewportContainer::PointDeleteRequested);
    connect(rightViewport_, &ViewportWidget::PointDeleteRequested,
            this, &ViewportContainer::PointDeleteRequested);

    // Set left viewport as active by default
    SetActiveViewport(leftViewport_);
}

ViewportContainer::~ViewportContainer() {
}

void ViewportContainer::SetProject(Project* project) {
    leftViewport_->SetProject(project);
    rightViewport_->SetProject(project);
}

void ViewportContainer::SetDualMode(bool dual) {
    dualMode_ = dual;

    if (dual) {
        // Dual viewport mode: target mesh left, morph mesh right
        leftViewport_->SetRenderFilter(RenderFilter::TargetOnly);
        leftViewport_->SetViewportLabel("Target Mesh");

        rightViewport_->SetRenderFilter(RenderFilter::MorphOnly);
        rightViewport_->SetViewportLabel("Morph Mesh");
        rightViewport_->show();

        // Set equal sizes for 50/50 split
        int totalWidth = splitter_->width();
        splitter_->setSizes({totalWidth / 2, totalWidth / 2});

        // Sync camera from left to right so they start at the same view
        rightViewport_->SyncCameraFrom(*leftViewport_->GetCamera());

        // Set left as active by default
        SetActiveViewport(leftViewport_);
    } else {
        // Single viewport mode: show all meshes
        rightViewport_->hide();
        leftViewport_->SetRenderFilter(RenderFilter::All);
        leftViewport_->SetViewportLabel("");
        leftViewport_->SetActive(false);
        leftViewport_->setStyleSheet(""); // Remove border in single mode
    }

    leftViewport_->update();
}

ViewportWidget* ViewportContainer::GetPrimaryViewport() {
    return leftViewport_;
}

ViewportWidget* ViewportContainer::GetSecondaryViewport() {
    return rightViewport_;
}

Camera* ViewportContainer::GetActiveCamera() {
    return leftViewport_->GetCamera();
}

void ViewportContainer::SetSelectedPointIndex(int index) {
    leftViewport_->SetSelectedPointIndex(index);
    rightViewport_->SetSelectedPointIndex(index);
}

void ViewportContainer::SetPointSize(float size) {
    leftViewport_->SetPointSize(size);
    rightViewport_->SetPointSize(size);
}

void ViewportContainer::OnLeftCameraChanged() {
    if (syncing_ || !dualMode_) return;

    syncing_ = true;

    // Set left as active viewport when user interacts with it
    SetActiveViewport(leftViewport_);

    // Sync left camera state to right viewport
    rightViewport_->SyncCameraFrom(*leftViewport_->GetCamera());

    syncing_ = false;
}

void ViewportContainer::OnRightCameraChanged() {
    if (syncing_ || !dualMode_) return;

    syncing_ = true;

    // Set right as active viewport when user interacts with it
    SetActiveViewport(rightViewport_);

    // Sync right camera state to left viewport
    leftViewport_->SyncCameraFrom(*rightViewport_->GetCamera());

    syncing_ = false;
}

void ViewportContainer::OnLeftViewportClicked() {
    SetActiveViewport(leftViewport_);
}

void ViewportContainer::OnRightViewportClicked() {
    SetActiveViewport(rightViewport_);
}

void ViewportContainer::SetActiveViewport(ViewportWidget* viewport) {
    if (!dualMode_) return;

    leftViewport_->SetActive(viewport == leftViewport_);
    rightViewport_->SetActive(viewport == rightViewport_);
}

} // namespace MetaVisage
