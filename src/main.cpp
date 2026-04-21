#include "ui/AppWindow.h"
#include "utils/Logger.h"

#include <GLFW/glfw3.h>
#include <QCoreApplication>
#include <QString>
#include <QStandardPaths>

int main(int /*argc*/, char** /*argv*/) {
    QCoreApplication::setOrganizationName("MetaVisage");
    QCoreApplication::setApplicationName("MetaVisage");

    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    MetaVisage::Logger::Instance().Initialize(logDir);
    MV_LOG_INFO("MetaVisage v2.0.0 starting");

    MetaVisage::AppWindow appWindow;
    if (!appWindow.Initialize(1600, 900, "MetaVisage")) {
        MV_LOG_CRITICAL("AppWindow failed to initialize");
        return 1;
    }

    MV_LOG_INFO("Application window shown");
    appWindow.Run();
    MV_LOG_INFO("MetaVisage exiting");
    return 0;
}
