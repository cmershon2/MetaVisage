#ifndef VIEWPORTCONTAINER_H
#define VIEWPORTCONTAINER_H

#include <QWidget>
#include <QSplitter>
#include "core/Types.h"

namespace MetaVisage {

class ViewportWidget;
class Project;
class Camera;

class ViewportContainer : public QWidget {
    Q_OBJECT

public:
    explicit ViewportContainer(QWidget *parent = nullptr);
    ~ViewportContainer();

    void SetProject(Project* project);

    // Switch between single viewport (alignment/morph/touchup) and dual viewport (point reference)
    void SetDualMode(bool dual);
    bool IsDualMode() const { return dualMode_; }

    // Access viewports
    ViewportWidget* GetPrimaryViewport();    // Always visible (left in dual mode)
    ViewportWidget* GetSecondaryViewport();  // Only visible in dual mode (right)
    Camera* GetActiveCamera();               // Returns active viewport's camera

    // Point selection synchronization
    void SetSelectedPointIndex(int index);

    // Point size synchronization
    void SetPointSize(float size);

signals:
    // Forwarded from active viewport
    void TransformModeChanged(TransformMode mode, AxisConstraint axis);
    void TargetTransformChanged();

    // Point signals forwarded from viewports
    void PointPlaced(PointSide side, Vector3 position, int vertexIndex);
    void PointSelected(int correspondenceIndex);
    void PointDeleteRequested();

private slots:
    void OnLeftCameraChanged();
    void OnRightCameraChanged();
    void OnLeftViewportClicked();
    void OnRightViewportClicked();

private:
    void SetActiveViewport(ViewportWidget* viewport);

    QSplitter* splitter_;
    ViewportWidget* leftViewport_;
    ViewportWidget* rightViewport_;
    bool dualMode_;
    bool syncing_;  // Prevents infinite loop during camera sync
};

} // namespace MetaVisage

#endif // VIEWPORTCONTAINER_H
