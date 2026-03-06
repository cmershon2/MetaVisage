#include "ui/ShortcutsDialog.h"
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

namespace MetaVisage {

ShortcutsDialog::ShortcutsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Keyboard Shortcuts");
    setMinimumSize(450, 500);
    resize(500, 600);

    setStyleSheet(
        "QDialog { background-color: #2C3E50; color: white; }"
        "QTableWidget { background-color: #34495E; color: white; gridline-color: #555; "
        "    border: 1px solid #555; selection-background-color: #3498DB; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background-color: #2C3E50; color: white; padding: 6px; "
        "    border: 1px solid #555; font-weight: bold; }"
        "QPushButton { background-color: #3498DB; color: white; border: none; "
        "    padding: 8px 24px; border-radius: 4px; font-size: 10pt; }"
        "QPushButton:hover { background-color: #2980B9; }"
    );

    QVBoxLayout* layout = new QVBoxLayout(this);

    QTableWidget* table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Shortcut", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setShowGrid(true);

    layout->addWidget(table);

    PopulateShortcuts();

    // Count total rows needed
    int rowCount = 0;
    // File (6), Edit (2), Alignment (6), Camera (8), Point Reference (2), Touch Up (2)
    rowCount = 6 + 6 + 2 + 6 + 8 + 2 + 2; // categories + items
    table->setRowCount(rowCount);

    int row = 0;
    AddCategory(table, "File", row);
    AddShortcut(table, "Ctrl+N", "New Project", row);
    AddShortcut(table, "Ctrl+O", "Open Project", row);
    AddShortcut(table, "Ctrl+S", "Save Project", row);
    AddShortcut(table, "Ctrl+Shift+S", "Save Project As", row);
    AddShortcut(table, "Ctrl+E", "Export Mesh", row);

    AddCategory(table, "Edit", row);
    AddShortcut(table, "Ctrl+Z", "Undo", row);
    AddShortcut(table, "Ctrl+Shift+Z", "Redo", row);

    AddCategory(table, "Alignment", row);
    AddShortcut(table, "G", "Move Tool", row);
    AddShortcut(table, "R", "Rotate Tool", row);
    AddShortcut(table, "S", "Scale Tool", row);
    AddShortcut(table, "X / Y / Z", "Constrain to Axis", row);
    AddShortcut(table, "Escape", "Cancel Transform", row);

    AddCategory(table, "Camera", row);
    AddShortcut(table, "Middle Mouse", "Orbit Camera", row);
    AddShortcut(table, "Shift+Middle Mouse", "Pan Camera", row);
    AddShortcut(table, "Scroll Wheel", "Zoom", row);
    AddShortcut(table, "Numpad 1", "Front View", row);
    AddShortcut(table, "Numpad 3", "Right View", row);
    AddShortcut(table, "Numpad 7", "Top View", row);
    AddShortcut(table, "Home", "Reset Camera", row);

    AddCategory(table, "Point Reference", row);
    AddShortcut(table, "Delete", "Delete Selected Point", row);

    AddCategory(table, "Touch Up", row);
    AddShortcut(table, "[ / ]", "Decrease / Increase Brush Radius", row);

    table->setRowCount(row);

    QPushButton* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

void ShortcutsDialog::PopulateShortcuts() {
    // Populated inline in constructor
}

void ShortcutsDialog::AddCategory(QTableWidget* table, const QString& category, int& row) {
    if (row >= table->rowCount()) table->setRowCount(row + 1);

    QTableWidgetItem* item = new QTableWidgetItem(category);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    QFont font = item->font();
    font.setBold(true);
    font.setPointSize(10);
    item->setFont(font);
    item->setBackground(QColor("#2C3E50"));
    item->setForeground(QColor("#3498DB"));
    table->setItem(row, 0, item);

    QTableWidgetItem* empty = new QTableWidgetItem("");
    empty->setFlags(empty->flags() & ~Qt::ItemIsSelectable);
    empty->setBackground(QColor("#2C3E50"));
    table->setItem(row, 1, empty);

    table->setSpan(row, 0, 1, 2);
    row++;
}

void ShortcutsDialog::AddShortcut(QTableWidget* table, const QString& shortcut, const QString& description, int& row) {
    if (row >= table->rowCount()) table->setRowCount(row + 1);

    QTableWidgetItem* keyItem = new QTableWidgetItem(shortcut);
    keyItem->setForeground(QColor("#F39C12"));
    QFont font = keyItem->font();
    font.setBold(true);
    keyItem->setFont(font);
    table->setItem(row, 0, keyItem);

    QTableWidgetItem* descItem = new QTableWidgetItem(description);
    table->setItem(row, 1, descItem);

    row++;
}

} // namespace MetaVisage
