#include "ui/WelcomeDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>
#include <QIcon>

namespace MetaVisage {

WelcomeDialog::WelcomeDialog(QWidget *parent)
    : QDialog(parent),
      result_(Cancelled) {
    setWindowTitle("Welcome to MetaVisage");
    setFixedSize(600, 450);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    SetupUI();
}

WelcomeDialog::~WelcomeDialog() {
}

void WelcomeDialog::SetupUI() {
    setStyleSheet("QDialog { background-color: #1E1E1E; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Banner area
    QWidget* banner = new QWidget();
    banner->setStyleSheet("QWidget { background-color: #2C3E50; }");
    banner->setFixedHeight(120);
    QHBoxLayout* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(30, 20, 30, 20);

    // App icon
    QLabel* iconLabel = new QLabel();
    QPixmap iconPixmap("assets/icons/Windows/Square44x44Logo.targetsize-64.png");
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    iconLabel->setFixedSize(64, 64);
    bannerLayout->addWidget(iconLabel);

    bannerLayout->addSpacing(16);

    // Title and tagline
    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel("MetaVisage");
    titleLabel->setStyleSheet("QLabel { color: white; font-size: 28pt; font-weight: bold; background: transparent; }");
    titleLayout->addWidget(titleLabel);

    QLabel* taglineLabel = new QLabel("MetaHuman Mesh Morphing Tool");
    taglineLabel->setStyleSheet("QLabel { color: #BDC3C7; font-size: 11pt; background: transparent; }");
    titleLayout->addWidget(taglineLabel);

    titleLayout->addStretch();
    bannerLayout->addLayout(titleLayout);
    bannerLayout->addStretch();

    mainLayout->addWidget(banner);

    // Content area
    QWidget* content = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(30, 20, 30, 20);
    contentLayout->setSpacing(12);

    // Recent Projects section
    QLabel* recentLabel = new QLabel("Recent Projects");
    recentLabel->setStyleSheet("QLabel { color: #BDC3C7; font-size: 12pt; font-weight: bold; }");
    contentLayout->addWidget(recentLabel);

    QStringList recentProjects = GetRecentProjects();

    if (recentProjects.isEmpty()) {
        QLabel* noRecent = new QLabel("No recent projects");
        noRecent->setStyleSheet("QLabel { color: #7F8C8D; font-style: italic; padding: 20px 0; }");
        noRecent->setAlignment(Qt::AlignCenter);
        contentLayout->addWidget(noRecent);
    } else {
        for (const QString& path : recentProjects) {
            QFileInfo fileInfo(path);
            if (!fileInfo.exists()) continue;

            QPushButton* projectButton = new QPushButton();
            projectButton->setCursor(Qt::PointingHandCursor);

            QString projectName = fileInfo.baseName();
            QString projectDir = fileInfo.absolutePath();
            QString lastModified = fileInfo.lastModified().toString("MMM d, yyyy h:mm AP");

            projectButton->setText(projectName);
            projectButton->setToolTip(path);
            projectButton->setStyleSheet(
                "QPushButton {"
                "    background-color: #2B2B2B;"
                "    color: white;"
                "    border: 1px solid #3C3C3C;"
                "    border-radius: 6px;"
                "    padding: 0px;"
                "    text-align: left;"
                "    font-size: 10pt;"
                "}"
                "QPushButton:hover {"
                "    background-color: #34495E;"
                "    border-color: #3498DB;"
                "}"
            );
            projectButton->setMinimumHeight(70);

            // Use a layout inside the button for multi-line display
            QVBoxLayout* btnLayout = new QVBoxLayout(projectButton);
            btnLayout->setContentsMargins(16, 10, 16, 10);
            btnLayout->setSpacing(3);

            // Clear default text, use layout labels instead
            projectButton->setText("");

            QLabel* nameLabel = new QLabel(projectName);
            nameLabel->setStyleSheet("QLabel { color: white; font-size: 10pt; font-weight: bold; background: transparent; }");
            nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            btnLayout->addWidget(nameLabel);

            QLabel* pathLabel = new QLabel(projectDir);
            pathLabel->setStyleSheet("QLabel { color: #7F8C8D; font-size: 8pt; background: transparent; }");
            pathLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            btnLayout->addWidget(pathLabel);

            QLabel* dateLabel = new QLabel("Last modified: " + lastModified);
            dateLabel->setStyleSheet("QLabel { color: #95A5A6; font-size: 8pt; background: transparent; }");
            dateLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            btnLayout->addWidget(dateLabel);

            connect(projectButton, &QPushButton::clicked, this, [this, path]() {
                OnRecentProjectClicked(path);
            });

            contentLayout->addWidget(projectButton);
        }
    }

    contentLayout->addStretch();

    // Bottom buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);

    QPushButton* newButton = new QPushButton("New Project");
    newButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498DB;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 24px;"
        "    font-size: 11pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980B9;"
        "}"
    );
    connect(newButton, &QPushButton::clicked, this, &WelcomeDialog::OnNewProject);
    buttonLayout->addWidget(newButton);

    QPushButton* openButton = new QPushButton("Open Project...");
    openButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #34495E;"
        "    color: white;"
        "    border: 1px solid #555;"
        "    padding: 10px 24px;"
        "    font-size: 11pt;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3C546A;"
        "}"
    );
    connect(openButton, &QPushButton::clicked, this, &WelcomeDialog::OnOpenProject);
    buttonLayout->addWidget(openButton);

    buttonLayout->addStretch();
    contentLayout->addLayout(buttonLayout);

    mainLayout->addWidget(content);
}

void WelcomeDialog::OnNewProject() {
    result_ = NewProject;
    accept();
}

void WelcomeDialog::OnOpenProject() {
    QString filepath = QFileDialog::getOpenFileName(
        this, tr("Open Project"), QString(), tr("MetaVisage Project (*.mmproj)"));

    if (!filepath.isEmpty()) {
        result_ = OpenProject;
        selectedPath_ = filepath;
        accept();
    }
}

void WelcomeDialog::OnRecentProjectClicked(const QString& path) {
    result_ = OpenRecent;
    selectedPath_ = path;
    accept();
}

QStringList WelcomeDialog::GetRecentProjects() {
    QSettings settings("MetaVisage", "MetaVisage");
    return settings.value("recentProjects").toStringList();
}

void WelcomeDialog::AddRecentProject(const QString& path) {
    QSettings settings("MetaVisage", "MetaVisage");
    QStringList recent = settings.value("recentProjects").toStringList();

    // Remove if already in list, then prepend
    recent.removeAll(path);
    recent.prepend(path);

    // Keep only last 5
    while (recent.size() > 5) {
        recent.removeLast();
    }

    settings.setValue("recentProjects", recent);
}

} // namespace MetaVisage
