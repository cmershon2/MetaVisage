#include "ui/MainWindow.h"
#include "ui/ViewportWidget.h"
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
      viewportWidget_(nullptr),
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
    toolsMenu->addAction(tr("Move"))->setShortcut(QKeySequence(Qt::Key_G));
    toolsMenu->addAction(tr("Rotate"))->setShortcut(QKeySequence(Qt::Key_R));
    toolsMenu->addAction(tr("Scale"))->setShortcut(QKeySequence(Qt::Key_S));

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
    // TODO: Add toolbar buttons in Sprint 3
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

    // Create viewport (75% width)
    viewportWidget_ = new ViewportWidget(centralWidget);
    layout->addWidget(viewportWidget_, 3);

    // Create sidebar (25% width)
    sidebarWidget_ = new SidebarWidget(centralWidget);
    sidebarWidget_->setMinimumWidth(320);
    sidebarWidget_->setMaximumWidth(400);
    layout->addWidget(sidebarWidget_, 1);

    setCentralWidget(centralWidget);
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
    // TODO: Implement new project dialog
    project_ = std::make_unique<Project>();
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
        // TODO: Load project
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
        // TODO: Save project
        QMessageBox::information(this, tr("Not Implemented"),
            tr("Project saving will be implemented in Sprint 10"));
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
    // TODO: Implement in Sprint 2
}

void MainWindow::OnResetCamera() {
    // TODO: Implement in Sprint 2
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
        "  Middle Mouse - Orbit\n"
        "  Shift+Middle Mouse - Pan\n"
        "  Scroll - Zoom";

    QMessageBox::information(this, tr("Keyboard Shortcuts"), shortcuts);
}

void MainWindow::OnReportIssue() {
    QMessageBox::information(this, tr("Report Issue"),
        tr("Please report issues at:\n"
           "https://github.com/YourUsername/MetaVisage/issues\n\n"
           "(GitHub repository will be created in Sprint 12)"));
}

void MainWindow::OnAbout() {
    QMessageBox::about(this, tr("About MetaVisage"),
        tr("<h3>MetaVisage v1.0.0</h3>"
           "<p>Open-source MetaHuman Mesh Morphing Tool</p>"
           "<p>Licensed under MIT License</p>"
           "<p>Author: Casey Mershon</p>"
           "<p>&copy; 2025</p>"));
}

} // namespace MetaVisage
