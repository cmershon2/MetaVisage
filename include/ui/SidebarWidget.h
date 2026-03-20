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
#include <QProgressBar>
#include <QButtonGroup>
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

    // Morph stage updates
    void OnMorphProgress(float progress, const QString& message);
    void OnMorphComplete(bool success, const QString& message);
    void SetMorphProcessing(bool processing);

    // Touch Up stage - update brush radius display from viewport
    void SetBrushRadius(float radius);

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
    // Handle morph parameter changes
    void OnStiffnessChanged(int value);
    void OnSmoothnessChanged(int value);
    void OnKernelTypeChanged(int index);
    void OnAlgorithmChanged(int index);
    void OnPreviewModeChanged(int index);
    // NRICP parameter changes
    void OnNRICPAlphaInitialChanged(int value);
    void OnNRICPAlphaFinalChanged(int value);
    void OnNRICPStiffnessStepsChanged(int value);
    void OnNRICPIcpIterationsChanged(int value);
    void OnNRICPNormalThresholdChanged(int value);
    void OnNRICPLandmarkWeightChanged(int value);
    void OnNRICPBoundaryExclusionToggled(bool enabled);
    void OnNRICPBoundaryHopsChanged(int value);
    // NRICP new feature parameters
    void OnNRICPOptimizationIterationsChanged(int value);
    void OnNRICPDpInitialChanged(int value);
    void OnNRICPDpFinalChanged(int value);
    void OnNRICPGammaInitialChanged(int value);
    void OnNRICPGammaFinalChanged(int value);
    void OnNRICPSamplingInitialChanged(int value);
    void OnNRICPSamplingFinalChanged(int value);
    void OnNRICPNormalizeSamplingToggled(bool enabled);
    // Handle sculpting parameter changes
    void OnBrushRadiusChanged(int value);
    void OnBrushStrengthChanged(int value);
    void OnFalloffTypeChanged(int index);

signals:
    void NextStageRequested();
    void PreviousStageRequested();
    void ImportMorphMeshRequested();
    void ImportTargetMeshRequested();
    void UseDefaultMorphMeshRequested();
    void ResetTransformRequested();
    void TransformValuesChanged();
    void ClearAllPointsRequested();
    void PointSelectedFromList(int correspondenceIndex);
    void PointSizeChanged(float size);
    void SymmetryChanged(bool enabled, Axis axis);

    // Morph stage signals
    void ProcessMorphRequested();
    void CancelMorphRequested();
    void AcceptMorphRequested();
    void MorphParameterChanged();
    void MorphPreviewModeChanged(MorphPreviewMode mode);

    // Mask painting signals (Morph stage)
    void MaskBrushToggled(bool active);
    void MaskBrushEraseModeChanged(bool erase);
    void MaskBrushRadiusChanged(float radius);
    void ClearMaskRequested();
    void InvertMaskRequested();

    // Touch Up stage signals
    void BrushTypeChanged(BrushType type);
    void BrushRadiusChangedSignal(float radius);
    void BrushStrengthChangedSignal(float strength);
    void BrushFalloffChanged(FalloffType falloff);
    void SculptSymmetryChanged(bool enabled, Axis axis);
    void ShowTargetOverlayChanged(bool show);
    void FinalizeRequested();

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
    QPushButton* prevStageButton_;
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

    // Morph stage UI elements (only valid during Morph stage)
    QComboBox* algorithmCombo_;
    QWidget* rbfParamsWidget_;
    QWidget* nricpParamsWidget_;
    QSlider* stiffnessSlider_;
    QSlider* smoothnessSlider_;
    QLabel* stiffnessValueLabel_;
    QLabel* smoothnessValueLabel_;
    QComboBox* kernelTypeCombo_;
    // NRICP parameter sliders
    QSlider* nricpAlphaInitialSlider_;
    QSlider* nricpAlphaFinalSlider_;
    QSlider* nricpStiffnessStepsSlider_;
    QSlider* nricpIcpIterationsSlider_;
    QSlider* nricpNormalThresholdSlider_;
    QSlider* nricpLandmarkWeightSlider_;
    QLabel* nricpAlphaInitialLabel_;
    QLabel* nricpAlphaFinalLabel_;
    QLabel* nricpStiffnessStepsLabel_;
    QLabel* nricpIcpIterationsLabel_;
    QLabel* nricpNormalThresholdLabel_;
    QLabel* nricpLandmarkWeightLabel_;
    QCheckBox* nricpBoundaryExclusionCheckBox_;
    QSlider* nricpBoundaryHopsSlider_;
    QLabel* nricpBoundaryHopsLabel_;
    // NRICP new feature sliders
    QSlider* nricpOptimizationIterationsSlider_;
    QLabel* nricpOptimizationIterationsLabel_;
    QSlider* nricpDpInitialSlider_;
    QLabel* nricpDpInitialLabel_;
    QSlider* nricpDpFinalSlider_;
    QLabel* nricpDpFinalLabel_;
    QSlider* nricpGammaInitialSlider_;
    QLabel* nricpGammaInitialLabel_;
    QSlider* nricpGammaFinalSlider_;
    QLabel* nricpGammaFinalLabel_;
    QSlider* nricpSamplingInitialSlider_;
    QLabel* nricpSamplingInitialLabel_;
    QSlider* nricpSamplingFinalSlider_;
    QLabel* nricpSamplingFinalLabel_;
    QCheckBox* nricpNormalizeSamplingCheckBox_;
    // Mask painting UI elements
    QPushButton* maskBrushButton_;
    QCheckBox* maskEraseModeCheckBox_;
    QSlider* maskBrushRadiusSlider_;
    QLabel* maskBrushRadiusLabel_;
    QPushButton* clearMaskButton_;
    QPushButton* invertMaskButton_;

    QPushButton* processButton_;
    QPushButton* cancelButton_;
    QProgressBar* progressBar_;
    QLabel* progressLabel_;
    QComboBox* previewModeCombo_;
    QPushButton* acceptButton_;
    QPushButton* reprocessButton_;
    QPushButton* resetDefaultsButton_;

    // Touch Up stage UI elements (only valid during TouchUp stage)
    QButtonGroup* brushButtonGroup_;
    QPushButton* smoothBrushButton_;
    QPushButton* grabBrushButton_;
    QPushButton* pushPullBrushButton_;
    QPushButton* inflateBrushButton_;
    QSlider* brushRadiusSlider_;
    QLabel* brushRadiusValueLabel_;
    QSlider* brushStrengthSlider_;
    QLabel* brushStrengthValueLabel_;
    QComboBox* falloffTypeCombo_;
    QCheckBox* sculptSymmetryCheckBox_;
    QComboBox* sculptSymmetryAxisCombo_;
    QCheckBox* showOverlayCheckBox_;
    QPushButton* finalizeButton_;

    // Flag to prevent feedback loops when updating spinboxes programmatically
    bool updatingTransformDisplay_;

    // Currently selected point in list
    int selectedPointIndex_;
};

} // namespace MetaVisage

#endif // SIDEBARWIDGET_H
