#include "ui/MainWindow.h"
#include "utils/Logger.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QStandardPaths>

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

    // Initialize logging system
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    MetaVisage::Logger::Instance().Initialize(logDir);
    MV_LOG_INFO("MetaVisage v1.0.0 starting");

    MetaVisage::MainWindow mainWindow;
    mainWindow.show();

    MV_LOG_INFO("Application window shown");
    return app.exec();
}
