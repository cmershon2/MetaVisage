#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QThread>
#include <QTimer>
#include <memory>
#include "core/Project.h"
#include "core/UndoStack.h"
#include "sculpting/BrushStroke.h"

namespace MetaVisage {

class ViewportWidget;
class ViewportContainer;
class SidebarWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    Project* GetProject() { return project_.get(); }

private slots:
    // File menu actions
    void OnNewProject();
    void OnOpenProject();
    void OnSaveProject();
    void OnSaveProjectAs();
    void OnImportMorphMesh();
    void OnImportTargetMesh();
    void OnUseDefaultMorphMesh();
    void OnExportMesh();
    void OnExit();

    // Edit menu actions
    void OnUndo();
    void OnRedo();
    void OnPreferences();

    // View menu actions
    void OnToggleGrid();
    void OnResetCamera();

    // Help menu actions
    void OnDocumentation();
    void OnKeyboardShortcuts();
    void OnReportIssue();
    void OnAbout();

    // Sidebar actions
    void OnNextStage();
    void OnPreviousStage();
    void OnResetTransform();
    void OnClearAllPoints();

    // Point reference actions
    void OnPointPlaced(PointSide side, Vector3 position, int vertexIndex);
    void OnPointSelected(int correspondenceIndex);
    void OnPointDeleteRequested();
    void OnPointSelectedFromList(int correspondenceIndex);
    void OnPointSizeChanged(float size);

    // Morph processing actions
    void OnProcessMorph();
    void OnCancelMorph();
    void OnAcceptMorph();
    void OnMorphProgress(float progress, const QString& message);
    void OnMorphComplete(bool success, const QString& errorMessage,
                         std::shared_ptr<Mesh> deformedMesh,
                         const std::vector<float>& displacements,
                         float maxDisplacement, float avgDisplacement);
    void OnMorphPreviewModeChanged(MorphPreviewMode mode);
    void OnMorphParameterChanged();

    // Texture import actions
    void OnImportAlbedoTexture();
    void OnImportNormalMap();

    // Touch Up actions
    void OnFinalizeRequested();

    // Undo/redo signals from viewport
    void OnTransformApplied(Transform before, Transform after);
    void OnSculptStrokeCompleted(BrushStroke stroke);

    // Stats timer
    void UpdateStatsDisplay();

private:
    void CreateMenus();
    void CreateToolBar();
    void CreateStatusBar();
    void CreateCentralWidget();
    void ShowWelcomeDialog();
    void UpdateWindowTitle();
    void UpdateStatusBar();
    void ConnectViewportSignals();
    void RefreshPointUI();
    void UpdateUndoRedoState();
    void BackupProject();

    // Symmetry helpers
    void PlaceSymmetricPoint(PointSide side, const Vector3& position, int vertexIndex, int parentIndex);

    // UI Components
    QMenuBar* menuBar_;
    QToolBar* toolBar_;
    QStatusBar* statusBar_;
    QLabel* statusLabel_;
    QLabel* toolLabel_;
    QLabel* statsLabel_;

    ViewportContainer* viewportContainer_;
    SidebarWidget* sidebarWidget_;

    // Project data
    std::unique_ptr<Project> project_;

    // Undo/Redo
    std::unique_ptr<UndoStack> undoStack_;
    QAction* undoAction_;
    QAction* redoAction_;

    // Stats timer
    QTimer* statsTimer_;
    float currentFPS_;

    // Morph processing
    QThread* morphThread_;
    class MorphWorker* morphWorker_;
};

} // namespace MetaVisage

#endif // MAINWINDOW_H
