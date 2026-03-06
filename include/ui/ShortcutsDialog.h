#ifndef SHORTCUTSDIALOG_H
#define SHORTCUTSDIALOG_H

#include <QDialog>

class QTableWidget;

namespace MetaVisage {

class ShortcutsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutsDialog(QWidget *parent = nullptr);

private:
    void PopulateShortcuts();
    void AddCategory(QTableWidget* table, const QString& category, int& row);
    void AddShortcut(QTableWidget* table, const QString& shortcut, const QString& description, int& row);
};

} // namespace MetaVisage

#endif // SHORTCUTSDIALOG_H
