#include "ui/MainWindow.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QStandardPaths>
#include <QIcon>

int main(int argc, char *argv[]) {
    // Set up OpenGL context format
    QSurfaceFormat format;
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // 4x MSAA
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName("MetaVisage");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MetaVisage");

    // Set application icon
    QIcon appIcon;
    appIcon.addFile("assets/icons/Windows/Square44x44Logo.targetsize-16.png", QSize(16, 16));
    appIcon.addFile("assets/icons/Windows/Square44x44Logo.targetsize-32.png", QSize(32, 32));
    appIcon.addFile("assets/icons/Windows/Square44x44Logo.targetsize-48.png", QSize(48, 48));
    appIcon.addFile("assets/icons/Windows/Square44x44Logo.targetsize-64.png", QSize(64, 64));
    appIcon.addFile("assets/icons/Windows/Square44x44Logo.targetsize-256.png", QSize(256, 256));
    app.setWindowIcon(appIcon);

    // Initialize logging system
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    MetaVisage::Logger::Instance().Initialize(logDir);
    MV_LOG_INFO("MetaVisage v1.0.0 starting");

    MetaVisage::MainWindow mainWindow;
    mainWindow.show();

    MV_LOG_INFO("Application window shown");
    return app.exec();
}
