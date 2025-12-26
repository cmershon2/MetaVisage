#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <memory>
#include "core/Project.h"

namespace MetaVisage {

class ViewportWidget;
class SidebarWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // File menu actions
    void OnNewProject();
    void OnOpenProject();
    void OnSaveProject();
    void OnSaveProjectAs();
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

private:
    void CreateMenus();
    void CreateToolBar();
    void CreateStatusBar();
    void CreateCentralWidget();
    void UpdateWindowTitle();
    void UpdateStatusBar();

    // UI Components
    QMenuBar* menuBar_;
    QToolBar* toolBar_;
    QStatusBar* statusBar_;
    QLabel* statusLabel_;
    QLabel* toolLabel_;
    QLabel* statsLabel_;

    ViewportWidget* viewportWidget_;
    SidebarWidget* sidebarWidget_;

    // Project data
    std::unique_ptr<Project> project_;
};

} // namespace MetaVisage

#endif // MAINWINDOW_H
