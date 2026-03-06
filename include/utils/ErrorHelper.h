#ifndef ERRORHELPER_H
#define ERRORHELPER_H

#include <QString>
#include <QWidget>

namespace MetaVisage {

namespace ErrorHelper {

// Show an error dialog with optional technical details (expandable)
void ShowError(QWidget* parent, const QString& title,
               const QString& userMessage, const QString& technicalDetail = "");

// Show a warning dialog
void ShowWarning(QWidget* parent, const QString& title, const QString& message);

// Show a confirmation dialog, returns true if user confirmed
bool ConfirmAction(QWidget* parent, const QString& title, const QString& message);

} // namespace ErrorHelper

} // namespace MetaVisage

#endif // ERRORHELPER_H
