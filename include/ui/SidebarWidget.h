#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
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

    // Point reference updates
    void UpdatePointCounts();
    void UpdatePointList();
    void SetSelectedPointIndex(int index);

public slots:
    // Update display when transform mode changes
    void OnTransformModeChanged(TransformMode mode, AxisConstraint axis);
    // Update display when target mesh transform changes
    void OnTargetTransformChanged();

private slots:
    // Handle spinbox value changes from user input
    void OnSpinBoxValueChanged();
    // Handle point size slider change
    void OnPointSizeChanged(int value);
    // Handle symmetry toggle
    void OnSymmetryToggled(bool enabled);
    // Handle symmetry axis change
    void OnSymmetryAxisChanged(int index);

signals:
    void NextStageRequested();
    void ResetTransformRequested();
    void TransformValuesChanged();
    void ClearAllPointsRequested();
    void PointSelectedFromList(int correspondenceIndex);
    void PointSizeChanged(float size);
    void SymmetryChanged(bool enabled, Axis axis);

private:
    void CreateAlignmentControls();
    void CreatePointReferenceControls();
    void CreateMorphControls();
    void CreateTouchUpControls();
    void ClearControls();
    void UpdateTransformDisplay();
    void RebuildPointListContent();

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
    QWidget* pointListContent_;
    QPushButton* clearAllPointsButton_;
    QSlider* pointSizeSlider_;
    QCheckBox* symmetryCheckBox_;
    QComboBox* symmetryAxisCombo_;

    // Flag to prevent feedback loops when updating spinboxes programmatically
    bool updatingTransformDisplay_;

    // Currently selected point in list
    int selectedPointIndex_;
};

} // namespace MetaVisage

#endif // SIDEBARWIDGET_H
