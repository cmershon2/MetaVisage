#include "ui/MainWindow.h"
#include "ui/ViewportWidget.h"
#include "ui/ViewportContainer.h"
#include "ui/SidebarWidget.h"
#include <QHBoxLayout>
#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeySequence>

namespace MetaVisage {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      menuBar_(nullptr),
      toolBar_(nullptr),
      statusBar_(nullptr),
      statusLabel_(nullptr),
      toolLabel_(nullptr),
      statsLabel_(nullptr),
      viewportContainer_(nullptr),
      sidebarWidget_(nullptr),
      project_(std::make_unique<Project>()) {

    setWindowTitle("MetaVisage - MetaHuman Mesh Morphing Tool");
    setMinimumSize(1280, 720);
    resize(1920, 1080);

    CreateMenus();
    CreateToolBar();
    CreateStatusBar();
    CreateCentralWidget();

    UpdateWindowTitle();
    UpdateStatusBar();
}

MainWindow::~MainWindow() {
}

void MainWindow::CreateMenus() {
    menuBar_ = menuBar();

    // File Menu
    QMenu* fileMenu = menuBar_->addMenu(tr("&File"));

    QAction* newAction = fileMenu->addAction(tr("&New Project"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::OnNewProject);

    QAction* openAction = fileMenu->addAction(tr("&Open Project"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::OnOpenProject);

    fileMenu->addSeparator();

    QAction* saveAction = fileMenu->addAction(tr("&Save Project"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::OnSaveProject);

    QAction* saveAsAction = fileMenu->addAction(tr("Save Project &As..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::OnSaveProjectAs);

    fileMenu->addSeparator();

    QAction* importMorphAction = fileMenu->addAction(tr("Import &Morph Mesh (MetaHuman)"));
    importMorphAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(importMorphAction, &QAction::triggered, this, &MainWindow::OnImportMorphMesh);

    QAction* importTargetAction = fileMenu->addAction(tr("Import &Target Mesh (Custom)"));
    importTargetAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(importTargetAction, &QAction::triggered, this, &MainWindow::OnImportTargetMesh);

    fileMenu->addSeparator();

    QAction* exportAction = fileMenu->addAction(tr("&Export Mesh"));
    exportAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(exportAction, &QAction::triggered, this, &MainWindow::OnExportMesh);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::OnExit);

    // Edit Menu
    QMenu* editMenu = menuBar_->addMenu(tr("&Edit"));

    QAction* undoAction = editMenu->addAction(tr("&Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &MainWindow::OnUndo);

    QAction* redoAction = editMenu->addAction(tr("&Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::OnRedo);

    editMenu->addSeparator();

    QAction* prefsAction = editMenu->addAction(tr("&Preferences"));
    connect(prefsAction, &QAction::triggered, this, &MainWindow::OnPreferences);

    // View Menu
    QMenu* viewMenu = menuBar_->addMenu(tr("&View"));

    QAction* gridAction = viewMenu->addAction(tr("Show/Hide &Grid"));
    connect(gridAction, &QAction::triggered, this, &MainWindow::OnToggleGrid);

    QAction* resetCameraAction = viewMenu->addAction(tr("&Reset Camera"));
    resetCameraAction->setShortcut(QKeySequence(Qt::Key_Home));
    connect(resetCameraAction, &QAction::triggered, this, &MainWindow::OnResetCamera);

    // Tools Menu
    QMenu* toolsMenu = menuBar_->addMenu(tr("&Tools"));
    QAction* moveAction = toolsMenu->addAction(tr("Move (G)"));
    connect(moveAction, &QAction::triggered, this, [this]() {
        ViewportWidget* vp = viewportContainer_->GetPrimaryViewport();
        if (vp) {
            vp->setFocus();
            vp->SetTransformMode(TransformMode::Move);
        }
    });

    QAction* rotateAction = toolsMenu->addAction(tr("Rotate (R)"));
    connect(rotateAction, &QAction::triggered, this, [this]() {
        ViewportWidget* vp = viewportContainer_->GetPrimaryViewport();
        if (vp) {
            vp->setFocus();
            vp->SetTransformMode(TransformMode::Rotate);
        }
    });

    QAction* scaleAction = toolsMenu->addAction(tr("Scale (S)"));
    connect(scaleAction, &QAction::triggered, this, [this]() {
        ViewportWidget* vp = viewportContainer_->GetPrimaryViewport();
        if (vp) {
            vp->setFocus();
            vp->SetTransformMode(TransformMode::Scale);
        }
    });

    // Help Menu
    QMenu* helpMenu = menuBar_->addMenu(tr("&Help"));

    QAction* docsAction = helpMenu->addAction(tr("&Documentation"));
    docsAction->setShortcut(QKeySequence::HelpContents);
    connect(docsAction, &QAction::triggered, this, &MainWindow::OnDocumentation);

    QAction* shortcutsAction = helpMenu->addAction(tr("&Keyboard Shortcuts"));
    connect(shortcutsAction, &QAction::triggered, this, &MainWindow::OnKeyboardShortcuts);

    helpMenu->addSeparator();

    QAction* reportAction = helpMenu->addAction(tr("&Report Issue"));
    connect(reportAction, &QAction::triggered, this, &MainWindow::OnReportIssue);

    QAction* aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::OnAbout);
}

void MainWindow::CreateToolBar() {
    toolBar_ = addToolBar(tr("Main Toolbar"));
    toolBar_->setMovable(false);
}

void MainWindow::CreateStatusBar() {
    statusBar_ = statusBar();

    toolLabel_ = new QLabel("Tool: None");
    statusLabel_ = new QLabel("Ready");
    statsLabel_ = new QLabel("Vertices: 0 | Faces: 0 | FPS: 0");

    statusBar_->addWidget(toolLabel_);
    statusBar_->addWidget(statusLabel_, 1);
    statusBar_->addPermanentWidget(statsLabel_);
}

void MainWindow::CreateCentralWidget() {
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create viewport container (75% width) - manages single/dual viewport modes
    viewportContainer_ = new ViewportContainer(centralWidget);
    viewportContainer_->SetProject(project_.get());
    layout->addWidget(viewportContainer_, 3);

    // Create sidebar (25% width)
    sidebarWidget_ = new SidebarWidget(centralWidget);
    sidebarWidget_->setMinimumWidth(320);
    sidebarWidget_->setMaximumWidth(400);
    sidebarWidget_->SetProject(project_.get());
    layout->addWidget(sidebarWidget_, 1);

    ConnectViewportSignals();

    setCentralWidget(centralWidget);
}

void MainWindow::ConnectViewportSignals() {
    // Connect sidebar next stage button
    connect(sidebarWidget_, &SidebarWidget::NextStageRequested, this, &MainWindow::OnNextStage);

    // Connect sidebar reset transform button
    connect(sidebarWidget_, &SidebarWidget::ResetTransformRequested, this, &MainWindow::OnResetTransform);

    // Connect sidebar clear all points button
    connect(sidebarWidget_, &SidebarWidget::ClearAllPointsRequested, this, &MainWindow::OnClearAllPoints);

    // Connect viewport container transform signals to sidebar
    connect(viewportContainer_, &ViewportContainer::TransformModeChanged,
            sidebarWidget_, &SidebarWidget::OnTransformModeChanged);
    connect(viewportContainer_, &ViewportContainer::TargetTransformChanged,
            sidebarWidget_, &SidebarWidget::OnTargetTransformChanged);

    // Connect sidebar spinbox changes to viewport update
    connect(sidebarWidget_, &SidebarWidget::TransformValuesChanged,
            viewportContainer_->GetPrimaryViewport(), QOverload<>::of(&ViewportWidget::update));
}

void MainWindow::UpdateWindowTitle() {
    QString title = "MetaVisage - " + project_->GetName();
    if (!project_->GetPath().isEmpty()) {
        title += " - " + project_->GetPath();
    }
    setWindowTitle(title);
}

void MainWindow::UpdateStatusBar() {
    QString stageText;
    switch (project_->GetCurrentStage()) {
        case WorkflowStage::Alignment:
            stageText = "Stage 1/4: Alignment";
            break;
        case WorkflowStage::PointReference:
            stageText = "Stage 2/4: Point Reference";
            break;
        case WorkflowStage::Morph:
            stageText = "Stage 3/4: Morph";
            break;
        case WorkflowStage::TouchUp:
            stageText = "Stage 4/4: Touch Up";
            break;
    }
    statusLabel_->setText(stageText);
}

// File menu slots
void MainWindow::OnNewProject() {
    project_ = std::make_unique<Project>();
    viewportContainer_->SetProject(project_.get());
    sidebarWidget_->SetProject(project_.get());
    viewportContainer_->SetDualMode(false);
    sidebarWidget_->SetStage(WorkflowStage::Alignment);
    UpdateWindowTitle();
    UpdateStatusBar();
}

void MainWindow::OnOpenProject() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Open Project"),
        QString(),
        tr("MetaVisage Project (*.mmproj)")
    );

    if (!filepath.isEmpty()) {
        QMessageBox::information(this, tr("Not Implemented"),
            tr("Project loading will be implemented in Sprint 10"));
    }
}

void MainWindow::OnSaveProject() {
    if (project_->GetPath().isEmpty()) {
        OnSaveProjectAs();
    } else {
        // TODO: Save project
    }
}

void MainWindow::OnSaveProjectAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        tr("Save Project As"),
        QString(),
        tr("MetaVisage Project (*.mmproj)")
    );

    if (!filepath.isEmpty()) {
        QMessageBox::information(this, tr("Not Implemented"),
            tr("Project saving will be implemented in Sprint 10"));
    }
}

void MainWindow::OnImportMorphMesh() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Import Morph Mesh (MetaHuman)"),
        QString(),
        tr("3D Models (*.fbx *.obj *.gltf *.glb)")
    );

    if (!filepath.isEmpty()) {
        MeshReference& morphMesh = project_->GetMorphMesh();
        morphMesh.mesh = std::make_shared<Mesh>();

        if (morphMesh.mesh->Load(filepath)) {
            morphMesh.filepath = filepath;
            morphMesh.isLoaded = true;
            // Apply -90 degree rotation around X-axis to correct MetaHuman orientation
            Transform initialTransform;
            Quaternion xRotation = Quaternion::FromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), -90.0f);
            initialTransform.SetRotation(xRotation);
            morphMesh.transform = initialTransform;

            statusLabel_->setText("Morph mesh loaded: " + morphMesh.mesh->GetName());

            // Update sidebar mesh status
            QLabel* morphStatus = sidebarWidget_->findChild<QLabel*>("morphStatus");
            if (morphStatus) {
                morphStatus->setText("Morph Mesh: " + morphMesh.mesh->GetName());
                morphStatus->setStyleSheet("QLabel { color: #2ECC71; }");
            }

            // Enable next stage button if both meshes are loaded
            sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());

            // Auto-focus camera on the loaded mesh
            const BoundingBox& bounds = morphMesh.mesh->GetBounds();
            viewportContainer_->GetPrimaryViewport()->GetCamera()->FocusOnBounds(bounds);

            // Update viewport
            viewportContainer_->GetPrimaryViewport()->update();
        } else {
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to load morph mesh: %1").arg(filepath));
        }
    }
}

void MainWindow::OnImportTargetMesh() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Import Target Mesh (Custom)"),
        QString(),
        tr("3D Models (*.fbx *.obj *.gltf *.glb)")
    );

    if (!filepath.isEmpty()) {
        MeshReference& targetMesh = project_->GetTargetMesh();
        targetMesh.mesh = std::make_shared<Mesh>();

        if (targetMesh.mesh->Load(filepath)) {
            targetMesh.filepath = filepath;
            targetMesh.isLoaded = true;
            targetMesh.transform = Transform(); // Identity transform

            statusLabel_->setText("Target mesh loaded: " + targetMesh.mesh->GetName());

            // Update sidebar mesh status
            QLabel* targetStatus = sidebarWidget_->findChild<QLabel*>("targetStatus");
            if (targetStatus) {
                targetStatus->setText("Target Mesh: " + targetMesh.mesh->GetName());
                targetStatus->setStyleSheet("QLabel { color: #2ECC71; }");
            }

            // Enable next stage button if both meshes are loaded
            sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());

            // Auto-focus camera on the loaded mesh
            const BoundingBox& bounds = targetMesh.mesh->GetBounds();
            viewportContainer_->GetPrimaryViewport()->GetCamera()->FocusOnBounds(bounds);

            // Update viewport
            viewportContainer_->GetPrimaryViewport()->update();
        } else {
            QMessageBox::critical(this, tr("Error"),
                tr("Failed to load target mesh: %1").arg(filepath));
        }
    }
}

void MainWindow::OnExportMesh() {
    QMessageBox::information(this, tr("Not Implemented"),
        tr("Mesh export will be implemented in Sprint 10"));
}

void MainWindow::OnExit() {
    close();
}

// Edit menu slots
void MainWindow::OnUndo() {
    // TODO: Implement in Sprint 11
}

void MainWindow::OnRedo() {
    // TODO: Implement in Sprint 11
}

void MainWindow::OnPreferences() {
    QMessageBox::information(this, tr("Not Implemented"),
        tr("Preferences will be implemented in future sprints"));
}

// View menu slots
void MainWindow::OnToggleGrid() {
    // TODO: Implement grid toggle
}

void MainWindow::OnResetCamera() {
    // TODO: Implement camera reset
}

// Help menu slots
void MainWindow::OnDocumentation() {
    QMessageBox::information(this, tr("Documentation"),
        tr("Documentation will be available after Sprint 12.\n\n"
           "Please refer to CLAUDE.md and docs/PRD.md for now."));
}

void MainWindow::OnKeyboardShortcuts() {
    QString shortcuts =
        "File:\n"
        "  Ctrl+N - New Project\n"
        "  Ctrl+O - Open Project\n"
        "  Ctrl+S - Save Project\n"
        "  Ctrl+M - Import Morph Mesh\n"
        "  Ctrl+T - Import Target Mesh\n"
        "  Ctrl+E - Export Mesh\n\n"
        "Edit:\n"
        "  Ctrl+Z - Undo\n"
        "  Ctrl+Shift+Z - Redo\n\n"
        "Tools:\n"
        "  G - Move\n"
        "  R - Rotate\n"
        "  S - Scale\n"
        "  X/Y/Z - Constrain to axis\n\n"
        "View:\n"
        "  Home - Reset Camera\n"
        "  1 - Front view\n"
        "  3 - Right view\n"
        "  7 - Top view\n"
        "  5 - Toggle Perspective/Ortho\n"
        "  F - Focus on selection\n"
        "  Middle Mouse - Orbit\n"
        "  Shift+Middle Mouse - Pan\n"
        "  Scroll - Zoom";

    QMessageBox::information(this, tr("Keyboard Shortcuts"), shortcuts);
}

void MainWindow::OnReportIssue() {
    QMessageBox::information(this, tr("Report Issue"),
        tr("Please report issues at:\n"
           "https://github.com/cmershon2/MetaVisage/issues"));
}

void MainWindow::OnAbout() {
    QMessageBox::about(this, tr("About MetaVisage"),
        tr("<h3>MetaVisage v1.0.0</h3>"
           "<p>Open-source MetaHuman Mesh Morphing Tool</p>"
           "<p>Licensed under MIT License</p>"
           "<p>Author: Casey Mershon</p>"
           "<p>&copy; 2025</p>"));
}

// Sidebar actions
void MainWindow::OnResetTransform() {
    if (project_ && project_->GetTargetMesh().isLoaded) {
        project_->GetTargetMesh().transform.Reset();
        ViewportWidget* vp = viewportContainer_->GetPrimaryViewport();
        vp->CancelTransform();
        vp->update();
        sidebarWidget_->OnTargetTransformChanged();
        statusLabel_->setText("Target transform reset");
    }
}

void MainWindow::OnClearAllPoints() {
    if (!project_) return;

    int result = QMessageBox::question(this, tr("Clear All Points"),
        tr("Are you sure you want to clear all correspondence points?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes) {
        project_->GetPointReferenceData().correspondences.clear();
        sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());
        viewportContainer_->GetPrimaryViewport()->update();
        if (viewportContainer_->IsDualMode()) {
            viewportContainer_->GetSecondaryViewport()->update();
        }
        statusLabel_->setText("All points cleared");
    }
}

void MainWindow::OnNextStage() {
    if (!project_->CanProceedToNextStage()) {
        QString message;
        switch (project_->GetCurrentStage()) {
            case WorkflowStage::Alignment:
                message = "Both morph mesh and target mesh must be loaded before proceeding.";
                break;
            case WorkflowStage::PointReference:
                message = "Point counts must match between meshes before proceeding.";
                break;
            case WorkflowStage::Morph:
                message = "Morph must be processed and accepted before proceeding.";
                break;
            default:
                message = "Cannot proceed to next stage.";
                break;
        }
        QMessageBox::warning(this, tr("Cannot Proceed"), message);
        return;
    }

    // Advance to next stage
    WorkflowStage currentStage = project_->GetCurrentStage();
    WorkflowStage nextStage = currentStage;

    switch (currentStage) {
        case WorkflowStage::Alignment:
            nextStage = WorkflowStage::PointReference;
            break;
        case WorkflowStage::PointReference:
            nextStage = WorkflowStage::Morph;
            break;
        case WorkflowStage::Morph:
            nextStage = WorkflowStage::TouchUp;
            break;
        case WorkflowStage::TouchUp:
            // Already at final stage
            return;
    }

    project_->SetCurrentStage(nextStage);
    sidebarWidget_->SetStage(nextStage);
    UpdateStatusBar();

    // Update viewport layout based on stage
    if (nextStage == WorkflowStage::PointReference) {
        viewportContainer_->SetDualMode(true);
    } else {
        viewportContainer_->SetDualMode(false);
    }

    viewportContainer_->GetPrimaryViewport()->update();
}

} // namespace MetaVisage
