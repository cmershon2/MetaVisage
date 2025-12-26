#include "ui/MainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>

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

    MetaVisage::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
