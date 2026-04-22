#include "ui/AppWindow.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "utils/Paths.h"
#include "utils/SingleInstance.h"

#include <GLFW/glfw3.h>

int main(int /*argc*/, char** /*argv*/) {
    MetaVisage::Logger::Instance().Initialize(MetaVisage::Paths::LogDir());
    MV_LOG_INFO("MetaVisage v2.0.0 starting");

    MetaVisage::Config::Instance().Load();

    MetaVisage::SingleInstance singleInstance;
    // Raise-to-front for a second launch is a UI-thread concern; the callback
    // just logs until AppWindow exposes a way to focus from a worker thread.
    if (!singleInstance.TryAcquire("raise", [](const std::string& /*payload*/) {
            MV_LOG_INFO("SingleInstance: second launch signalled");
        })) {
        MV_LOG_INFO("MetaVisage already running; exiting second instance");
        return 0;
    }

    MetaVisage::AppWindow appWindow;
    const int width = MetaVisage::Config::Instance().WindowWidth();
    const int height = MetaVisage::Config::Instance().WindowHeight();
    if (!appWindow.Initialize(width, height, "MetaVisage")) {
        MV_LOG_CRITICAL("AppWindow failed to initialize");
        return 1;
    }

    MV_LOG_INFO("Application window shown");
    appWindow.Run();
    MV_LOG_INFO("MetaVisage exiting");
    return 0;
}
