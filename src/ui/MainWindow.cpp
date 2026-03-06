#include "ui/MainWindow.h"
#include "ui/ViewportWidget.h"
#include "ui/ViewportContainer.h"
#include "ui/SidebarWidget.h"
#include "ui/ExportDialog.h"
#include "ui/ShortcutsDialog.h"
#include "utils/RayCaster.h"
#include "utils/Logger.h"
#include "utils/ErrorHelper.h"
#include "deformation/MeshDeformer.h"
#include "deformation/NRICPDeformer.h"
#include "io/MeshExporter.h"
#include "io/ProjectSerializer.h"
#include "core/UndoActions.h"
#include <QHBoxLayout>
#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeySequence>
#include <QThread>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

namespace MetaVisage {

// Worker object that runs MeshDeformer on a background thread
class MorphWorker : public QObject {
    Q_OBJECT
public:
    MorphWorker(std::shared_ptr<Mesh> sourceMesh,
                std::shared_ptr<Mesh> targetMesh,
                const Transform& morphTransform,
                const Transform& targetTransform,
                const std::vector<PointCorrespondence>& correspondences,
                DeformationAlgorithm kernel, float stiffness, float smoothness,
                const NRICPParams& nricpParams = NRICPParams())
        : sourceMesh_(sourceMesh), targetMesh_(targetMesh),
          morphTransform_(morphTransform), targetTransform_(targetTransform),
          correspondences_(correspondences),
          kernel_(kernel), stiffness_(stiffness), smoothness_(smoothness),
          nricpParams_(nricpParams) {}

    MeshDeformer& GetDeformer() { return deformer_; }

public slots:
    void process() {
        deformer_.SetSourceMesh(sourceMesh_);
        deformer_.SetTargetMesh(targetMesh_);
        deformer_.SetMorphTransform(morphTransform_);
        deformer_.SetTargetTransform(targetTransform_);
        deformer_.SetCorrespondences(correspondences_);
        deformer_.SetKernelType(kernel_);
        deformer_.SetStiffness(stiffness_);
        deformer_.SetSmoothness(smoothness_);
        deformer_.SetNRICPParams(nricpParams_);
        deformer_.SetProgressCallback([this](float progress, const std::string& message) {
            emit progressUpdated(progress, QString::fromStdString(message));
        });

        DeformationResult result = deformer_.Deform();

        emit finished(result.success,
                      QString::fromStdString(result.errorMessage),
                      result.deformedMesh,
                      deformer_.GetDisplacementMagnitudes(),
                      result.maxDisplacement,
                      result.avgDisplacement);
    }

signals:
    void progressUpdated(float progress, const QString& message);
    void finished(bool success, const QString& errorMessage,
                  std::shared_ptr<Mesh> deformedMesh,
                  const std::vector<float>& displacements,
                  float maxDisplacement, float avgDisplacement);

private:
    MeshDeformer deformer_;
    std::shared_ptr<Mesh> sourceMesh_;
    std::shared_ptr<Mesh> targetMesh_;
    Transform morphTransform_;
    Transform targetTransform_;
    std::vector<PointCorrespondence> correspondences_;
    DeformationAlgorithm kernel_;
    float stiffness_;
    float smoothness_;
    NRICPParams nricpParams_;
};

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
      project_(std::make_unique<Project>()),
      undoStack_(std::make_unique<UndoStack>()),
      undoAction_(nullptr),
      redoAction_(nullptr),
      statsTimer_(nullptr),
      currentFPS_(0.0f),
      morphThread_(nullptr),
      morphWorker_(nullptr) {

    setWindowTitle("MetaVisage - MetaHuman Mesh Morphing Tool");
    setMinimumSize(1280, 720);
    resize(1920, 1080);

    CreateMenus();
    CreateToolBar();
    CreateStatusBar();
    CreateCentralWidget();

    // Stats timer for updating FPS and vertex/face counts
    statsTimer_ = new QTimer(this);
    connect(statsTimer_, &QTimer::timeout, this, &MainWindow::UpdateStatsDisplay);
    statsTimer_->start(500);

    UpdateWindowTitle();
    UpdateStatusBar();
    UpdateUndoRedoState();
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

    undoAction_ = editMenu->addAction(tr("&Undo"));
    undoAction_->setShortcut(QKeySequence::Undo);
    connect(undoAction_, &QAction::triggered, this, &MainWindow::OnUndo);

    redoAction_ = editMenu->addAction(tr("&Redo"));
    redoAction_->setShortcut(QKeySequence::Redo);
    connect(redoAction_, &QAction::triggered, this, &MainWindow::OnRedo);

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

    // Connect point signals from viewport container
    connect(viewportContainer_, &ViewportContainer::PointPlaced,
            this, &MainWindow::OnPointPlaced);
    connect(viewportContainer_, &ViewportContainer::PointSelected,
            this, &MainWindow::OnPointSelected);
    connect(viewportContainer_, &ViewportContainer::PointDeleteRequested,
            this, &MainWindow::OnPointDeleteRequested);

    // Connect sidebar point list selection
    connect(sidebarWidget_, &SidebarWidget::PointSelectedFromList,
            this, &MainWindow::OnPointSelectedFromList);

    // Connect sidebar point size slider
    connect(sidebarWidget_, &SidebarWidget::PointSizeChanged,
            this, &MainWindow::OnPointSizeChanged);

    // Connect morph stage signals
    connect(sidebarWidget_, &SidebarWidget::ProcessMorphRequested,
            this, &MainWindow::OnProcessMorph);
    connect(sidebarWidget_, &SidebarWidget::CancelMorphRequested,
            this, &MainWindow::OnCancelMorph);
    connect(sidebarWidget_, &SidebarWidget::AcceptMorphRequested,
            this, &MainWindow::OnAcceptMorph);
    connect(sidebarWidget_, &SidebarWidget::MorphParameterChanged,
            this, &MainWindow::OnMorphParameterChanged);
    connect(sidebarWidget_, &SidebarWidget::MorphPreviewModeChanged,
            this, &MainWindow::OnMorphPreviewModeChanged);

    // Connect Touch Up sculpting signals
    connect(sidebarWidget_, &SidebarWidget::BrushTypeChanged,
            viewportContainer_->GetPrimaryViewport(), &ViewportWidget::SetBrushType);
    connect(sidebarWidget_, &SidebarWidget::BrushRadiusChangedSignal,
            viewportContainer_->GetPrimaryViewport(), &ViewportWidget::SetBrushRadius);
    connect(sidebarWidget_, &SidebarWidget::BrushStrengthChangedSignal,
            viewportContainer_->GetPrimaryViewport(), &ViewportWidget::SetBrushStrength);
    connect(sidebarWidget_, &SidebarWidget::BrushFalloffChanged,
            viewportContainer_->GetPrimaryViewport(), &ViewportWidget::SetBrushFalloff);

    // Connect viewport brush radius change (from [ ] keys) back to sidebar
    connect(viewportContainer_->GetPrimaryViewport(), &ViewportWidget::BrushRadiusChanged,
            sidebarWidget_, &SidebarWidget::SetBrushRadius);

    // Connect sculpting symmetry signals
    connect(sidebarWidget_, &SidebarWidget::SculptSymmetryChanged,
            viewportContainer_->GetPrimaryViewport(), &ViewportWidget::SetSculptSymmetry);

    // Connect display options signals
    connect(sidebarWidget_, &SidebarWidget::ShowTargetOverlayChanged,
            viewportContainer_->GetPrimaryViewport(), &ViewportWidget::SetShowTargetOverlay);

    // Connect finalize button
    connect(sidebarWidget_, &SidebarWidget::FinalizeRequested,
            this, &MainWindow::OnFinalizeRequested);

    // Connect undo signals from viewport
    connect(viewportContainer_->GetPrimaryViewport(), &ViewportWidget::TransformApplied,
            this, &MainWindow::OnTransformApplied);
    connect(viewportContainer_->GetPrimaryViewport(), &ViewportWidget::SculptStrokeCompleted,
            this, &MainWindow::OnSculptStrokeCompleted);

    // Connect FPS signal
    connect(viewportContainer_->GetPrimaryViewport(), &ViewportWidget::FPSUpdated,
            this, [this](float fps) { currentFPS_ = fps; });
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
    undoStack_->Clear();
    viewportContainer_->SetProject(project_.get());
    sidebarWidget_->SetProject(project_.get());
    viewportContainer_->SetDualMode(false);
    sidebarWidget_->SetStage(WorkflowStage::Alignment);
    UpdateWindowTitle();
    UpdateStatusBar();
    UpdateUndoRedoState();
}

void MainWindow::OnOpenProject() {
    QString filepath = QFileDialog::getOpenFileName(
        this, tr("Open Project"), QString(), tr("MetaVisage Project (*.mmproj)"));

    if (!filepath.isEmpty()) {
        auto newProject = std::make_unique<Project>();
        if (newProject->Load(filepath)) {
            project_ = std::move(newProject);
            viewportContainer_->SetProject(project_.get());
            sidebarWidget_->SetProject(project_.get());

            // Restore viewport mode based on stage
            WorkflowStage stage = project_->GetCurrentStage();
            viewportContainer_->SetDualMode(stage == WorkflowStage::PointReference);
            sidebarWidget_->SetStage(stage);

            // Reconstruct deformed mesh from saved vertex data
            MorphData& morphData = project_->GetMorphData();
            if (morphData.hasDeformedData && morphData.isProcessed &&
                project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {

                // Store original morph mesh reference
                morphData.originalMorphMesh = project_->GetMorphMesh().mesh;

                // Create deformed mesh as a copy of the original, then replace vertices/normals
                auto deformed = std::make_shared<Mesh>();
                const Mesh& original = *project_->GetMorphMesh().mesh;
                deformed->SetVertices(morphData.savedDeformedVertices);
                deformed->SetNormals(morphData.savedDeformedNormals);
                deformed->SetUVs(original.GetUVs());
                deformed->SetFaces(original.GetFaces());
                deformed->SetMaterials(original.GetMaterials());
                deformed->SetName(original.GetName());

                morphData.deformedMorphMesh = deformed;

                // Clear the temporary saved data to free memory
                morphData.savedDeformedVertices.clear();
                morphData.savedDeformedNormals.clear();
                morphData.hasDeformedData = false;
            }

            // Set up Touch Up stage if that's where we loaded into
            if (stage == WorkflowStage::TouchUp) {
                viewportContainer_->GetPrimaryViewport()->SetViewportLabel("Touch Up");
            }

            // Focus camera on loaded mesh
            if (project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {
                const BoundingBox& bounds = project_->GetMorphMesh().mesh->GetBounds();
                viewportContainer_->GetPrimaryViewport()->GetCamera()->FocusOnBounds(bounds);
            }

            // Update sidebar mesh labels
            if (project_->GetMorphMesh().isLoaded) {
                QLabel* morphStatus = sidebarWidget_->findChild<QLabel*>("morphStatus");
                if (morphStatus) {
                    morphStatus->setText("Morph Mesh: " + project_->GetMorphMesh().mesh->GetName());
                    morphStatus->setStyleSheet("QLabel { color: #2ECC71; }");
                }
            }
            if (project_->GetTargetMesh().isLoaded) {
                QLabel* targetStatus = sidebarWidget_->findChild<QLabel*>("targetStatus");
                if (targetStatus) {
                    targetStatus->setText("Target Mesh: " + project_->GetTargetMesh().mesh->GetName());
                    targetStatus->setStyleSheet("QLabel { color: #2ECC71; }");
                }
            }

            UpdateWindowTitle();
            UpdateStatusBar();
            sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());
            viewportContainer_->GetPrimaryViewport()->update();

            statusLabel_->setText("Project loaded: " + project_->GetName());
        } else {
            ErrorHelper::ShowError(this, tr("Load Failed"),
                tr("Failed to load project."), filepath);
        }
    }
}

void MainWindow::OnSaveProject() {
    if (project_->GetPath().isEmpty()) {
        OnSaveProjectAs();
        return;
    }

    if (project_->Save(project_->GetPath())) {
        UpdateWindowTitle();
        statusLabel_->setText("Project saved: " + project_->GetPath());
    } else {
        ErrorHelper::ShowError(this, tr("Save Failed"),
            tr("Failed to save project."), project_->GetPath());
    }
}

void MainWindow::OnSaveProjectAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this, tr("Save Project As"), QString(), tr("MetaVisage Project (*.mmproj)"));

    if (!filepath.isEmpty()) {
        if (!filepath.endsWith(".mmproj", Qt::CaseInsensitive)) {
            filepath += ".mmproj";
        }

        if (project_->Save(filepath)) {
            // Update project name from filename
            QFileInfo info(filepath);
            project_->SetName(info.completeBaseName());
            UpdateWindowTitle();
            statusLabel_->setText("Project saved: " + filepath);
        } else {
            ErrorHelper::ShowError(this, tr("Save Failed"),
                tr("Failed to save project."), filepath);
        }
    }
}

void MainWindow::OnImportMorphMesh() {
    QString filepath = QFileDialog::getOpenFileName(
        this, tr("Import Morph Mesh (MetaHuman)"), QString(),
        tr("3D Models (*.fbx *.obj *.gltf *.glb)"));

    if (!filepath.isEmpty()) {
        MeshReference& morphMesh = project_->GetMorphMesh();
        morphMesh.mesh = std::make_shared<Mesh>();

        if (morphMesh.mesh->Load(filepath)) {
            morphMesh.filepath = filepath;
            morphMesh.isLoaded = true;
            Transform initialTransform;
            Quaternion xRotation = Quaternion::FromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), -90.0f);
            initialTransform.SetRotation(xRotation);
            morphMesh.transform = initialTransform;

            statusLabel_->setText("Morph mesh loaded: " + morphMesh.mesh->GetName());

            QLabel* morphStatus = sidebarWidget_->findChild<QLabel*>("morphStatus");
            if (morphStatus) {
                morphStatus->setText("Morph Mesh: " + morphMesh.mesh->GetName());
                morphStatus->setStyleSheet("QLabel { color: #2ECC71; }");
            }

            sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());

            const BoundingBox& bounds = morphMesh.mesh->GetBounds();
            viewportContainer_->GetPrimaryViewport()->GetCamera()->FocusOnBounds(bounds);
            viewportContainer_->GetPrimaryViewport()->update();
        } else {
            ErrorHelper::ShowError(this, tr("Import Failed"),
                tr("Failed to load morph mesh."), filepath);
        }
    }
}

void MainWindow::OnImportTargetMesh() {
    QString filepath = QFileDialog::getOpenFileName(
        this, tr("Import Target Mesh (Custom)"), QString(),
        tr("3D Models (*.fbx *.obj *.gltf *.glb)"));

    if (!filepath.isEmpty()) {
        MeshReference& targetMesh = project_->GetTargetMesh();
        targetMesh.mesh = std::make_shared<Mesh>();

        if (targetMesh.mesh->Load(filepath)) {
            targetMesh.filepath = filepath;
            targetMesh.isLoaded = true;
            targetMesh.transform = Transform();

            statusLabel_->setText("Target mesh loaded: " + targetMesh.mesh->GetName());

            QLabel* targetStatus = sidebarWidget_->findChild<QLabel*>("targetStatus");
            if (targetStatus) {
                targetStatus->setText("Target Mesh: " + targetMesh.mesh->GetName());
                targetStatus->setStyleSheet("QLabel { color: #2ECC71; }");
            }

            sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());

            const BoundingBox& bounds = targetMesh.mesh->GetBounds();
            viewportContainer_->GetPrimaryViewport()->GetCamera()->FocusOnBounds(bounds);
            viewportContainer_->GetPrimaryViewport()->update();
        } else {
            ErrorHelper::ShowError(this, tr("Import Failed"),
                tr("Failed to load target mesh."), filepath);
        }
    }
}

void MainWindow::OnExportMesh() {
    // Determine which mesh to export (prefer deformed, fall back to morph)
    const MorphData& morphData = project_->GetMorphData();
    const Mesh* meshToExport = nullptr;
    const Transform* exportTransform = nullptr;

    if (morphData.deformedMorphMesh) {
        meshToExport = morphData.deformedMorphMesh.get();
        exportTransform = &project_->GetMorphMesh().transform;
    } else if (project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {
        meshToExport = project_->GetMorphMesh().mesh.get();
        exportTransform = &project_->GetMorphMesh().transform;
    }

    if (!meshToExport) {
        ErrorHelper::ShowWarning(this, tr("No Mesh"),
            tr("No mesh available to export. Please load or process a mesh first."));
        return;
    }

    // Show export dialog
    ExportDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString filepath = dialog.GetFilePath();
    ExportOptions options = dialog.GetOptions();

    // Show progress
    QProgressDialog progress("Exporting mesh...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    // Export
    MeshExporter exporter;
    exporter.SetProgressCallback([&progress](float p, const QString& msg) {
        progress.setValue(static_cast<int>(p * 100.0f));
        progress.setLabelText(msg);
    });

    ExportResult result = exporter.Export(*meshToExport, filepath, options, exportTransform);

    progress.setValue(100);

    if (result.success) {
        QString message = QString("Mesh exported successfully!\n\n"
                                  "File: %1\n"
                                  "Vertices: %2\n"
                                  "Faces: %3")
                              .arg(result.exportedFilePath)
                              .arg(result.vertexCount)
                              .arg(result.faceCount);

        QMessageBox::StandardButton reply = QMessageBox::information(
            this, tr("Export Complete"), message + "\n\nOpen export folder?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            QFileInfo info(result.exportedFilePath);
            QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
        }

        statusLabel_->setText("Mesh exported to: " + result.exportedFilePath);
    } else {
        ErrorHelper::ShowError(this, tr("Export Failed"),
            tr("Failed to export mesh."), result.errorMessage);
    }
}

void MainWindow::OnExit() {
    close();
}

// Edit menu slots
void MainWindow::OnUndo() {
    if (!undoStack_->CanUndo()) return;

    QString desc = undoStack_->UndoDescription();
    undoStack_->Undo();

    // Refresh UI after undo
    sidebarWidget_->OnTargetTransformChanged();
    RefreshPointUI();

    // Re-upload mesh vertex data to GPU if sculpt vertices were modified
    if (project_->GetCurrentStage() == WorkflowStage::TouchUp) {
        MorphData& morphData = project_->GetMorphData();
        if (morphData.deformedMorphMesh) {
            viewportContainer_->GetPrimaryViewport()->RefreshMeshGPUData(
                morphData.deformedMorphMesh.get());
        }
    }

    viewportContainer_->GetPrimaryViewport()->update();
    if (viewportContainer_->IsDualMode()) {
        viewportContainer_->GetSecondaryViewport()->update();
    }
    sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());
    UpdateUndoRedoState();
    statusLabel_->setText(QString("Undo: %1").arg(desc));
}

void MainWindow::OnRedo() {
    if (!undoStack_->CanRedo()) return;

    QString desc = undoStack_->RedoDescription();
    undoStack_->Redo();

    // Refresh UI after redo
    sidebarWidget_->OnTargetTransformChanged();
    RefreshPointUI();

    // Re-upload mesh vertex data to GPU if sculpt vertices were modified
    if (project_->GetCurrentStage() == WorkflowStage::TouchUp) {
        MorphData& morphData = project_->GetMorphData();
        if (morphData.deformedMorphMesh) {
            viewportContainer_->GetPrimaryViewport()->RefreshMeshGPUData(
                morphData.deformedMorphMesh.get());
        }
    }

    viewportContainer_->GetPrimaryViewport()->update();
    if (viewportContainer_->IsDualMode()) {
        viewportContainer_->GetSecondaryViewport()->update();
    }
    sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());
    UpdateUndoRedoState();
    statusLabel_->setText(QString("Redo: %1").arg(desc));
}

void MainWindow::UpdateUndoRedoState() {
    if (undoAction_) {
        undoAction_->setEnabled(undoStack_->CanUndo());
        undoAction_->setText(undoStack_->CanUndo()
            ? QString("&Undo %1").arg(undoStack_->UndoDescription())
            : tr("&Undo"));
    }
    if (redoAction_) {
        redoAction_->setEnabled(undoStack_->CanRedo());
        redoAction_->setText(undoStack_->CanRedo()
            ? QString("&Redo %1").arg(undoStack_->RedoDescription())
            : tr("&Redo"));
    }
}

void MainWindow::OnTransformApplied(Transform before, Transform after) {
    undoStack_->Push(std::make_unique<TransformUndoAction>(project_.get(), before, after));
    UpdateUndoRedoState();
}

void MainWindow::OnSculptStrokeCompleted(BrushStroke stroke) {
    if (stroke.IsEmpty()) return;

    Mesh* mesh = nullptr;
    MorphData& morphData = project_->GetMorphData();
    if (morphData.deformedMorphMesh) {
        mesh = morphData.deformedMorphMesh.get();
    }
    if (!mesh) return;

    // Capture after-positions for redo
    const auto& vertices = mesh->GetVertices();
    std::map<int, Vector3> afterPositions;
    // We need to get the original positions from the stroke to know which indices changed
    // The stroke already has the original positions stored; we get the after positions from current mesh
    // We use the BrushStroke internals via a small trick: undo, capture originals, then re-apply
    // Instead, just capture current positions for the same indices
    // Since BrushStroke stores originalPositions_ as private, we iterate by undoing then re-applying
    // Simpler approach: store current vertex positions for all indices the stroke modified
    auto tempVertices = vertices; // copy current state
    stroke.Undo(tempVertices); // this gives us the "before" state - but we already have it in the stroke
    // The after positions are just the current vertex positions
    // We need to know which indices the stroke modified. We can get the count but not the indices from public API.
    // Let's just undo to get the before-indices, compare, store the after-positions.

    // Actually, let's use a simpler approach: capture all vertices that differ between
    // current state and undo state
    for (size_t i = 0; i < vertices.size() && i < tempVertices.size(); ++i) {
        const Vector3& current = vertices[i];
        const Vector3& before = tempVertices[i];
        if (current.x != before.x || current.y != before.y || current.z != before.z) {
            afterPositions[static_cast<int>(i)] = current;
        }
    }

    undoStack_->Push(std::make_unique<SculptStrokeUndoAction>(mesh, stroke, afterPositions));
    UpdateUndoRedoState();
}

void MainWindow::BackupProject() {
    if (project_->GetPath().isEmpty()) return;
    QString backupPath = project_->GetPath() + ".backup";
    ProjectSerializer serializer;
    serializer.Save(*project_, backupPath);
    MV_LOG_INFO(QString("Project backed up to: %1").arg(backupPath));
}

void MainWindow::UpdateStatsDisplay() {
    size_t totalVertices = 0;
    size_t totalFaces = 0;

    if (project_) {
        if (project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {
            totalVertices += project_->GetMorphMesh().mesh->GetVertexCount();
            totalFaces += project_->GetMorphMesh().mesh->GetFaceCount();
        }
        if (project_->GetTargetMesh().isLoaded && project_->GetTargetMesh().mesh) {
            totalVertices += project_->GetTargetMesh().mesh->GetVertexCount();
            totalFaces += project_->GetTargetMesh().mesh->GetFaceCount();
        }
        const MorphData& morph = project_->GetMorphData();
        if (morph.deformedMorphMesh) {
            totalVertices += morph.deformedMorphMesh->GetVertexCount();
            totalFaces += morph.deformedMorphMesh->GetFaceCount();
        }
    }

    statsLabel_->setText(QString("Vertices: %1 | Faces: %2 | FPS: %3")
        .arg(totalVertices).arg(totalFaces).arg(static_cast<int>(currentFPS_)));
}

void MainWindow::OnPreferences() {
    QMessageBox::information(this, tr("Not Implemented"),
        tr("Preferences will be implemented in future sprints"));
}

// View menu slots
void MainWindow::OnToggleGrid() {}
void MainWindow::OnResetCamera() {}

// Help menu slots
void MainWindow::OnDocumentation() {
    QMessageBox::information(this, tr("Documentation"),
        tr("Documentation will be available after Sprint 12.\n\n"
           "Please refer to CLAUDE.md and docs/PRD.md for now."));
}

void MainWindow::OnKeyboardShortcuts() {
    ShortcutsDialog dialog(this);
    dialog.exec();
}

void MainWindow::OnReportIssue() {
    QDesktopServices::openUrl(QUrl("https://github.com/cmershon2/MetaVisage/issues"));
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
        Transform before = project_->GetTargetMesh().transform;
        project_->GetTargetMesh().transform.Reset();
        Transform after = project_->GetTargetMesh().transform;

        undoStack_->Push(std::make_unique<TransformUndoAction>(
            project_.get(), before, after));
        UpdateUndoRedoState();

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
        auto& correspondences = project_->GetPointReferenceData().correspondences;
        auto beforeCorrespondences = correspondences;

        correspondences.clear();

        undoStack_->Push(std::make_unique<PointPlacementUndoAction>(
            project_.get(), beforeCorrespondences, correspondences));
        UpdateUndoRedoState();

        viewportContainer_->SetSelectedPointIndex(-1);
        RefreshPointUI();
        statusLabel_->setText("All points cleared");
    }
}

// Point reference actions
void MainWindow::OnPointPlaced(PointSide side, Vector3 position, int vertexIndex) {
    if (!project_) return;

    auto& correspondences = project_->GetPointReferenceData().correspondences;

    // Capture before state for undo
    auto beforeCorrespondences = correspondences;

    // Determine the next point ID
    int nextID = 1;
    for (const auto& corr : correspondences) {
        if (corr.pointID >= nextID) {
            nextID = corr.pointID + 1;
        }
    }

    int modifiedIndex = -1;

    if (side == PointSide::Target) {
        // Look for a correspondence that's missing a target point
        for (int i = 0; i < static_cast<int>(correspondences.size()); ++i) {
            if (correspondences[i].targetMeshVertexIndex < 0 && correspondences[i].morphMeshVertexIndex >= 0) {
                correspondences[i].targetMeshPosition = position;
                correspondences[i].targetMeshVertexIndex = vertexIndex;
                modifiedIndex = i;
                break;
            }
        }

        if (modifiedIndex < 0) {
            PointCorrespondence newCorr;
            newCorr.pointID = nextID;
            newCorr.targetMeshPosition = position;
            newCorr.targetMeshVertexIndex = vertexIndex;
            correspondences.push_back(newCorr);
            modifiedIndex = static_cast<int>(correspondences.size()) - 1;
        }
    } else {
        // Look for a correspondence that's missing a morph point
        for (int i = 0; i < static_cast<int>(correspondences.size()); ++i) {
            if (correspondences[i].morphMeshVertexIndex < 0 && correspondences[i].targetMeshVertexIndex >= 0) {
                correspondences[i].morphMeshPosition = position;
                correspondences[i].morphMeshVertexIndex = vertexIndex;
                modifiedIndex = i;
                break;
            }
        }

        if (modifiedIndex < 0) {
            PointCorrespondence newCorr;
            newCorr.pointID = nextID;
            newCorr.morphMeshPosition = position;
            newCorr.morphMeshVertexIndex = vertexIndex;
            correspondences.push_back(newCorr);
            modifiedIndex = static_cast<int>(correspondences.size()) - 1;
        }
    }

    // Handle symmetry
    if (project_->GetPointReferenceData().symmetryEnabled) {
        PlaceSymmetricPoint(side, position, vertexIndex, modifiedIndex);
    }

    // Push undo action
    undoStack_->Push(std::make_unique<PointPlacementUndoAction>(
        project_.get(), beforeCorrespondences, correspondences, "Place Point"));
    UpdateUndoRedoState();

    RefreshPointUI();
    statusLabel_->setText(QString("Point %1 placed on %2 mesh")
        .arg(correspondences[modifiedIndex].pointID)
        .arg(side == PointSide::Target ? "target" : "morph"));
}

void MainWindow::PlaceSymmetricPoint(PointSide side, const Vector3& position, int /*vertexIndex*/, int parentIndex) {
    if (!project_) return;

    auto& pointRef = project_->GetPointReferenceData();
    Axis symAxis = pointRef.symmetryAxis;

    // Mirror the position across the symmetry axis
    Vector3 mirroredPos = position;
    switch (symAxis) {
        case Axis::X: mirroredPos.x = -mirroredPos.x; break;
        case Axis::Y: mirroredPos.y = -mirroredPos.y; break;
        case Axis::Z: mirroredPos.z = -mirroredPos.z; break;
    }

    // Find nearest vertex on the mesh for the mirrored position
    const Mesh* mesh = nullptr;
    const Transform* transform = nullptr;

    if (side == PointSide::Target && project_->GetTargetMesh().mesh) {
        mesh = project_->GetTargetMesh().mesh.get();
        transform = &project_->GetTargetMesh().transform;
    } else if (side == PointSide::Morph && project_->GetMorphMesh().mesh) {
        mesh = project_->GetMorphMesh().mesh.get();
        transform = &project_->GetMorphMesh().transform;
    }

    if (!mesh || !transform) return;

    // Use RayCaster to find nearest vertex
    int mirroredVertexIdx = RayCaster::FindNearestVertex(mirroredPos, *mesh, *transform);
    if (mirroredVertexIdx < 0) return;

    auto& correspondences = pointRef.correspondences;
    auto& parent = correspondences[parentIndex];

    // If the parent already has a symmetric pair, fill that pair's missing side data
    if (parent.isSymmetric && parent.symmetricPairID >= 0) {
        for (auto& corr : correspondences) {
            if (corr.pointID == parent.symmetricPairID) {
                if (side == PointSide::Target) {
                    corr.targetMeshPosition = mirroredPos;
                    corr.targetMeshVertexIndex = mirroredVertexIdx;
                } else {
                    corr.morphMeshPosition = mirroredPos;
                    corr.morphMeshVertexIndex = mirroredVertexIdx;
                }
                return;
            }
        }
    }

    // No existing symmetric pair - create a new correspondence
    int nextID = 1;
    for (const auto& corr : correspondences) {
        if (corr.pointID >= nextID) nextID = corr.pointID + 1;
    }

    PointCorrespondence symCorr;
    symCorr.pointID = nextID;
    symCorr.isSymmetric = true;
    symCorr.symmetricPairID = parent.pointID;

    if (side == PointSide::Target) {
        symCorr.targetMeshPosition = mirroredPos;
        symCorr.targetMeshVertexIndex = mirroredVertexIdx;
    } else {
        symCorr.morphMeshPosition = mirroredPos;
        symCorr.morphMeshVertexIndex = mirroredVertexIdx;
    }

    correspondences.push_back(symCorr);

    // Mark the parent as symmetric too
    // Re-fetch parent reference since push_back may have invalidated it
    correspondences[parentIndex].isSymmetric = true;
    correspondences[parentIndex].symmetricPairID = symCorr.pointID;
}

void MainWindow::OnPointSelected(int correspondenceIndex) {
    viewportContainer_->SetSelectedPointIndex(correspondenceIndex);
    sidebarWidget_->SetSelectedPointIndex(correspondenceIndex);
}

void MainWindow::OnPointDeleteRequested() {
    if (!project_) return;

    auto& correspondences = project_->GetPointReferenceData().correspondences;
    int selectedIdx = viewportContainer_->GetPrimaryViewport()->GetSelectedPointIndex();

    // Capture before state for undo
    auto beforeCorrespondences = correspondences;

    if (selectedIdx < 0 || selectedIdx >= static_cast<int>(correspondences.size())) return;

    // If this point has a symmetric pair, remove that too
    const auto& corr = correspondences[selectedIdx];
    if (corr.isSymmetric && corr.symmetricPairID >= 0) {
        int pairID = corr.symmetricPairID;
        // Find and remove the pair
        for (auto it = correspondences.begin(); it != correspondences.end(); ++it) {
            if (it->pointID == pairID) {
                correspondences.erase(it);
                break;
            }
        }
        // Recalculate selectedIdx since the vector may have shifted
        if (selectedIdx >= static_cast<int>(correspondences.size())) {
            selectedIdx = static_cast<int>(correspondences.size()) - 1;
        }
    }

    // Remove the selected point (recalculate position in case pair was removed before it)
    if (selectedIdx >= 0 && selectedIdx < static_cast<int>(correspondences.size())) {
        correspondences.erase(correspondences.begin() + selectedIdx);
    }

    // Renumber remaining points sequentially
    for (size_t i = 0; i < correspondences.size(); ++i) {
        correspondences[i].pointID = static_cast<int>(i) + 1;
    }

    // Clear selection
    viewportContainer_->SetSelectedPointIndex(-1);
    sidebarWidget_->SetSelectedPointIndex(-1);

    // Push undo action
    undoStack_->Push(std::make_unique<PointPlacementUndoAction>(
        project_.get(), beforeCorrespondences, correspondences, "Delete Point"));
    UpdateUndoRedoState();

    RefreshPointUI();
    statusLabel_->setText("Point deleted");
}

void MainWindow::OnPointSelectedFromList(int correspondenceIndex) {
    viewportContainer_->SetSelectedPointIndex(correspondenceIndex);
}

void MainWindow::OnPointSizeChanged(float size) {
    viewportContainer_->SetPointSize(size);
}

void MainWindow::RefreshPointUI() {
    sidebarWidget_->UpdatePointList();
    viewportContainer_->GetPrimaryViewport()->update();
    if (viewportContainer_->IsDualMode()) {
        viewportContainer_->GetSecondaryViewport()->update();
    }
}

// Morph processing slots
void MainWindow::OnProcessMorph() {
    if (!project_ || morphThread_) return;

    // Backup project before morphing
    BackupProject();

    MorphData& morphData = project_->GetMorphData();
    const MeshReference& morphMeshRef = project_->GetMorphMesh();

    if (!morphMeshRef.isLoaded || !morphMeshRef.mesh) {
        ErrorHelper::ShowWarning(this, tr("Error"), tr("Morph mesh is not loaded."));
        return;
    }

    // Store original morph mesh if not already stored
    if (!morphData.originalMorphMesh) {
        morphData.originalMorphMesh = morphMeshRef.mesh;
    }

    // Invalidate any previously cached deformed mesh in renderer
    if (morphData.deformedMorphMesh) {
        viewportContainer_->GetPrimaryViewport()->InvalidateMesh(morphData.deformedMorphMesh.get());
    }

    // Reset processing state
    morphData.isProcessed = false;
    morphData.isAccepted = false;

    // Get target mesh reference
    const MeshReference& targetMeshRef = project_->GetTargetMesh();
    if (!targetMeshRef.isLoaded || !targetMeshRef.mesh) {
        ErrorHelper::ShowWarning(this, tr("Error"), tr("Target mesh is not loaded."));
        return;
    }

    // Build NRICP parameters from project data
    NRICPParams nricpParams;
    nricpParams.stiffnessSteps = morphData.nricpStiffnessSteps;
    nricpParams.alphaInitial = morphData.nricpAlphaInitial;
    nricpParams.alphaFinal = morphData.nricpAlphaFinal;
    nricpParams.icpIterations = morphData.nricpIcpIterations;
    nricpParams.normalThreshold = morphData.nricpNormalThreshold;
    nricpParams.landmarkWeight = morphData.nricpLandmarkWeight;
    nricpParams.epsilon = morphData.nricpEpsilon;

    // Create worker and thread, passing transforms for coordinate space conversion
    morphThread_ = new QThread(this);
    morphWorker_ = new MorphWorker(
        morphData.originalMorphMesh,
        targetMeshRef.mesh,
        morphMeshRef.transform,
        targetMeshRef.transform,
        project_->GetPointReferenceData().correspondences,
        morphData.algorithm,
        morphData.stiffness,
        morphData.smoothness,
        nricpParams
    );
    morphWorker_->moveToThread(morphThread_);

    // Connect signals
    connect(morphThread_, &QThread::started, morphWorker_, &MorphWorker::process);
    connect(morphWorker_, &MorphWorker::progressUpdated, this, &MainWindow::OnMorphProgress);
    connect(morphWorker_, &MorphWorker::finished, this, &MainWindow::OnMorphComplete);
    connect(morphWorker_, &MorphWorker::finished, morphThread_, &QThread::quit);
    connect(morphThread_, &QThread::finished, morphWorker_, &QObject::deleteLater);
    connect(morphThread_, &QThread::finished, morphThread_, &QObject::deleteLater);
    connect(morphThread_, &QThread::finished, this, [this]() {
        morphThread_ = nullptr;
        morphWorker_ = nullptr;
    });

    // Update UI state
    sidebarWidget_->SetMorphProcessing(true);
    statusLabel_->setText("Processing morph...");

    // Start processing
    morphThread_->start();
}

void MainWindow::OnCancelMorph() {
    if (morphWorker_) {
        morphWorker_->GetDeformer().Cancel();
        statusLabel_->setText("Cancelling morph...");
    }
}

void MainWindow::OnAcceptMorph() {
    if (!project_) return;

    MorphData& morphData = project_->GetMorphData();
    morphData.isAccepted = true;
    sidebarWidget_->SetNextStageEnabled(project_->CanProceedToNextStage());
    statusLabel_->setText("Morph result accepted");
}

void MainWindow::OnMorphProgress(float progress, const QString& message) {
    sidebarWidget_->OnMorphProgress(progress, message);
}

void MainWindow::OnMorphComplete(bool success, const QString& errorMessage,
                                  std::shared_ptr<Mesh> deformedMesh,
                                  const std::vector<float>& displacements,
                                  float maxDisplacement, float avgDisplacement) {
    MorphData& morphData = project_->GetMorphData();

    if (success && deformedMesh) {
        // Capture before state for undo
        MorphUndoAction::MorphState beforeState;
        beforeState.deformedMesh = morphData.deformedMorphMesh;
        beforeState.isProcessed = morphData.isProcessed;
        beforeState.isAccepted = morphData.isAccepted;
        beforeState.displacementMagnitudes = morphData.displacementMagnitudes;
        beforeState.maxDisplacement = morphData.maxDisplacement;
        beforeState.avgDisplacement = morphData.avgDisplacement;

        morphData.deformedMorphMesh = deformedMesh;
        morphData.displacementMagnitudes = displacements;
        morphData.maxDisplacement = maxDisplacement;
        morphData.avgDisplacement = avgDisplacement;
        morphData.isProcessed = true;
        morphData.isAccepted = false;

        // Upload heat map colors for the deformed mesh
        viewportContainer_->GetPrimaryViewport()->UploadHeatMapColors(
            deformedMesh.get(), displacements, maxDisplacement);

        // Capture after state and push undo
        MorphUndoAction::MorphState afterState;
        afterState.deformedMesh = morphData.deformedMorphMesh;
        afterState.isProcessed = morphData.isProcessed;
        afterState.isAccepted = morphData.isAccepted;
        afterState.displacementMagnitudes = morphData.displacementMagnitudes;
        afterState.maxDisplacement = morphData.maxDisplacement;
        afterState.avgDisplacement = morphData.avgDisplacement;

        undoStack_->Push(std::make_unique<MorphUndoAction>(
            project_.get(), beforeState, afterState));
        UpdateUndoRedoState();

        sidebarWidget_->OnMorphComplete(true,
            QString("Done! Max displacement: %1, Avg: %2")
                .arg(maxDisplacement, 0, 'f', 4)
                .arg(avgDisplacement, 0, 'f', 4));
        statusLabel_->setText("Morph processing complete");
    } else {
        sidebarWidget_->OnMorphComplete(false,
            errorMessage.isEmpty() ? "Processing cancelled" : errorMessage);
        statusLabel_->setText("Morph processing failed");
    }

    viewportContainer_->GetPrimaryViewport()->update();
}

void MainWindow::OnMorphPreviewModeChanged(MorphPreviewMode mode) {
    if (!project_) return;
    project_->GetMorphData().previewMode = mode;
    viewportContainer_->GetPrimaryViewport()->SetMorphPreviewMode(mode);
    viewportContainer_->GetPrimaryViewport()->update();
}

void MainWindow::OnMorphParameterChanged() {
    if (!project_) return;

    MorphData& morphData = project_->GetMorphData();

    // Parameters already updated by sidebar directly on project
    // Mark as needing re-processing
    if (morphData.isProcessed) {
        morphData.isProcessed = false;
        morphData.isAccepted = false;
        sidebarWidget_->SetNextStageEnabled(false);
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
        ErrorHelper::ShowWarning(this, tr("Cannot Proceed"), message);
        return;
    }

    // Backup project before stage transition
    BackupProject();

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
            return;
    }

    // Clear undo stack when transitioning stages
    undoStack_->Clear();
    UpdateUndoRedoState();

    project_->SetCurrentStage(nextStage);
    sidebarWidget_->SetStage(nextStage);
    UpdateStatusBar();

    // Update viewport layout based on stage
    if (nextStage == WorkflowStage::PointReference) {
        viewportContainer_->SetDualMode(true);
    } else {
        viewportContainer_->SetDualMode(false);
    }

    // When entering Morph stage, store original morph mesh copy
    if (nextStage == WorkflowStage::Morph) {
        MorphData& morphData = project_->GetMorphData();
        if (!morphData.originalMorphMesh) {
            morphData.originalMorphMesh = project_->GetMorphMesh().mesh;
        }
    }

    // When entering Touch Up stage, set viewport label
    if (nextStage == WorkflowStage::TouchUp) {
        viewportContainer_->GetPrimaryViewport()->SetViewportLabel("Touch Up");
        statusLabel_->setText("Touch Up - Use sculpting brushes to refine the morph");
    }

    viewportContainer_->GetPrimaryViewport()->update();
}

void MainWindow::OnFinalizeRequested() {
    if (!project_) return;

    const MorphData& morphData = project_->GetMorphData();
    Mesh* mesh = morphData.deformedMorphMesh ? morphData.deformedMorphMesh.get() : nullptr;

    if (!mesh) {
        mesh = project_->GetMorphMesh().mesh.get();
    }

    if (!mesh) {
        ErrorHelper::ShowWarning(this, tr("Error"), tr("No mesh available to finalize."));
        return;
    }

    // Perform mesh validation
    const auto& vertices = mesh->GetVertices();
    const auto& faces = mesh->GetFaces();
    int vertexCount = static_cast<int>(vertices.size());
    int faceCount = static_cast<int>(faces.size());
    int degenerateFaces = 0;

    // Check for degenerate faces (duplicate vertex indices)
    for (const auto& face : faces) {
        const auto& idx = face.vertexIndices;
        if (idx.size() < 3) {
            degenerateFaces++;
            continue;
        }
        // Check for out-of-range or duplicate indices
        bool degenerate = false;
        for (size_t i = 0; i < idx.size() && !degenerate; ++i) {
            if (idx[i] >= vertices.size()) degenerate = true;
            for (size_t j = i + 1; j < idx.size() && !degenerate; ++j) {
                if (idx[i] == idx[j]) degenerate = true;
            }
        }
        if (degenerate) degenerateFaces++;
    }

    QString validationResult;
    bool valid = true;

    validationResult += QString("Mesh Validation Report\n\n");
    validationResult += QString("Vertices: %1\n").arg(vertexCount);
    validationResult += QString("Faces: %1\n").arg(faceCount);

    if (degenerateFaces > 0) {
        validationResult += QString("\nWarning: %1 degenerate faces detected.\n").arg(degenerateFaces);
        valid = false;
    }

    if (valid) {
        validationResult += "\nMesh passes all validation checks!\n";
        validationResult += "\nReady for export.";
    } else {
        validationResult += "\nMesh has issues but can still be exported.";
    }

    QMessageBox::StandardButton result = QMessageBox::information(
        this, tr("Mesh Validation"),
        validationResult + "\n\nProceed to export?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        OnExportMesh();
    }
}

} // namespace MetaVisage

// MOC include for Q_OBJECT class defined in this .cpp file
#include "MainWindow.moc"
