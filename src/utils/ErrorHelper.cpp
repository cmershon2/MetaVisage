#include "utils/ErrorHelper.h"
#include "utils/Logger.h"
#include <QMessageBox>

namespace MetaVisage {

namespace ErrorHelper {

void ShowError(QWidget* parent, const QString& title,
               const QString& userMessage, const QString& technicalDetail) {
    MV_LOG_ERROR(QString("%1: %2").arg(title, userMessage));
    if (!technicalDetail.isEmpty()) {
        MV_LOG_ERROR(QString("  Detail: %1").arg(technicalDetail));
    }

    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle(title);
    msgBox.setText(userMessage);
    if (!technicalDetail.isEmpty()) {
        msgBox.setDetailedText(technicalDetail);
    }
    msgBox.exec();
}

void ShowWarning(QWidget* parent, const QString& title, const QString& message) {
    MV_LOG_WARNING(QString("%1: %2").arg(title, message));

    QMessageBox::warning(parent, title, message);
}

bool ConfirmAction(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox::StandardButton result =
        QMessageBox::question(parent, title, message,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);
    return result == QMessageBox::Yes;
}

} // namespace ErrorHelper

} // namespace MetaVisage
