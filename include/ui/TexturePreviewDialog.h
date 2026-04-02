#ifndef TEXTUREPREVIEWDIALOG_H
#define TEXTUREPREVIEWDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include "core/TextureData.h"

namespace MetaVisage {

class TexturePreviewDialog : public QDialog {
    Q_OBJECT

public:
    TexturePreviewDialog(const QString& textureName,
                         const TextureData& texture,
                         QWidget* parent = nullptr);
    ~TexturePreviewDialog() = default;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void UpdatePreview();

    QLabel* imageLabel_;
    QLabel* infoLabel_;
    QImage fullImage_;
};

} // namespace MetaVisage

#endif // TEXTUREPREVIEWDIALOG_H
