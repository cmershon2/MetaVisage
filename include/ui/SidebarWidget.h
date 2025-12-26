#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "core/Types.h"

namespace MetaVisage {

class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    ~SidebarWidget();

    void SetStage(WorkflowStage stage);

private:
    void CreateAlignmentControls();
    void CreatePointReferenceControls();
    void CreateMorphControls();
    void CreateTouchUpControls();
    void ClearControls();

    QVBoxLayout* layout_;
    QLabel* stageLabel_;
    QPushButton* nextStageButton_;
    QWidget* controlsWidget_;
};

} // namespace MetaVisage

#endif // SIDEBARWIDGET_H
