#include "ui/TexturePreviewDialog.h"
#include <QPixmap>
#include <QResizeEvent>
#include <QFileInfo>

namespace MetaVisage {

TexturePreviewDialog::TexturePreviewDialog(const QString& textureName,
                                           const TextureData& texture,
                                           QWidget* parent)
    : QDialog(parent) {

    setWindowTitle(QString("Texture Preview: %1").arg(textureName));
    setMinimumSize(300, 300);
    resize(600, 650);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // Info label
    QFileInfo fi(texture.GetFilePath());
    QString info = QString("%1  |  %2 x %3  |  %4")
        .arg(textureName)
        .arg(texture.GetWidth())
        .arg(texture.GetHeight())
        .arg(fi.fileName());

    infoLabel_ = new QLabel(info);
    infoLabel_->setStyleSheet("QLabel { color: #ECF0F1; font-size: 10pt; padding: 4px; }");
    infoLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel_);

    // Image display
    imageLabel_ = new QLabel();
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setStyleSheet("QLabel { background-color: #1a1a1a; border: 1px solid #555; }");
    imageLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(imageLabel_, 1);

    // Load the full image
    fullImage_ = texture.ToQImage();

    // Style the dialog
    setStyleSheet(
        "QDialog {"
        "    background-color: #2C3E50;"
        "}"
    );

    UpdatePreview();
}

void TexturePreviewDialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    UpdatePreview();
}

void TexturePreviewDialog::UpdatePreview() {
    if (fullImage_.isNull() || !imageLabel_) return;

    QSize availableSize = imageLabel_->size();
    if (availableSize.width() < 10 || availableSize.height() < 10) return;

    QPixmap pixmap = QPixmap::fromImage(fullImage_);
    QPixmap scaled = pixmap.scaled(availableSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel_->setPixmap(scaled);
}

} // namespace MetaVisage
