#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include "core/Types.h"

namespace MetaVisage {

class Project;

class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    ~SidebarWidget();

    void SetStage(WorkflowStage stage);
    void SetNextStageEnabled(bool enabled) { nextStageButton_->setEnabled(enabled); }
    void SetProject(Project* project) { project_ = project; }

public slots:
    // Update display when transform mode changes
    void OnTransformModeChanged(TransformMode mode, AxisConstraint axis);
    // Update display when target mesh transform changes
    void OnTargetTransformChanged();

private slots:
    // Handle spinbox value changes from user input
    void OnSpinBoxValueChanged();

signals:
    void NextStageRequested();
    void ResetTransformRequested();
    void TransformValuesChanged();
    void ClearAllPointsRequested();

private:
    void CreateAlignmentControls();
    void CreatePointReferenceControls();
    void CreateMorphControls();
    void CreateTouchUpControls();
    void ClearControls();
    void UpdateTransformDisplay();

    Project* project_;

    QVBoxLayout* layout_;
    QLabel* stageLabel_;
    QPushButton* nextStageButton_;
    QWidget* controlsWidget_;

    // Transform tool UI elements (only valid during Alignment stage)
    QLabel* transformModeLabel_;
    QDoubleSpinBox* posXSpinBox_;
    QDoubleSpinBox* posYSpinBox_;
    QDoubleSpinBox* posZSpinBox_;
    QDoubleSpinBox* rotXSpinBox_;
    QDoubleSpinBox* rotYSpinBox_;
    QDoubleSpinBox* rotZSpinBox_;
    QDoubleSpinBox* scaleXSpinBox_;
    QDoubleSpinBox* scaleYSpinBox_;
    QDoubleSpinBox* scaleZSpinBox_;
    QPushButton* resetTransformButton_;

    // Point reference UI elements (only valid during PointReference stage)
    QLabel* targetPointCountLabel_;
    QLabel* morphPointCountLabel_;
    QLabel* matchStatusLabel_;
    QScrollArea* pointListScroll_;
    QPushButton* clearAllPointsButton_;

    // Flag to prevent feedback loops when updating spinboxes programmatically
    bool updatingTransformDisplay_;
};

} // namespace MetaVisage

#endif // SIDEBARWIDGET_H
