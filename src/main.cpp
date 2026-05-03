#include "ui/AppWindow.h"
#include "licensing/LicenseManager.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "utils/Paths.h"
#include "utils/SingleInstance.h"

#include <GLFW/glfw3.h>

#include <string>
#include <string_view>

namespace {
// PR #25 review fix: shared constant so the argv pre-filter and the
// SingleInstance forwarder agree on the scheme prefix. Was a magic
// `13` in the argv check at one site and `kScheme.size()` in the other.
constexpr std::string_view kDeepLinkScheme = "metavisage://";
}

int main(int argc, char** argv) {
    MetaVisage::Logger::Instance().Initialize(MetaVisage::Paths::LogDir());
    MV_LOG_INFO("MetaVisage v2.0.0 starting");

    MetaVisage::Config::Instance().Load();

    // Sprint 6 Story 6.2 -- the OS launches us with argv[1] = the
    // metavisage:// deep-link URI when the scheme handler fires. Parse it
    // before SingleInstance so the second-instance forwarding path can
    // hand the full URI to the first instance via the named-pipe payload.
    std::string deepLink;
    if (argc > 1 && argv[1]) {
        std::string_view arg(argv[1]);
        if (arg.size() > kDeepLinkScheme.size() &&
            arg.substr(0, kDeepLinkScheme.size()) == kDeepLinkScheme) {
            deepLink.assign(arg);
            MV_LOG_INFO("main: deep-link URI received via argv[1]");
        }
    }

    MetaVisage::SingleInstance singleInstance;
    // Payload to the first instance: the full URI on a deep-link launch,
    // otherwise the legacy "raise" sentinel. The onRaise callback runs on
    // SingleInstance's listener thread; LicenseManager::QueueDeepLink is
    // thread-safe and the main thread drains the queue each frame.
    const std::string payload = deepLink.empty() ? std::string("raise") : deepLink;
    if (!singleInstance.TryAcquire(payload, [](const std::string& msg) {
            if (msg.size() > kDeepLinkScheme.size() &&
                msg.substr(0, kDeepLinkScheme.size()) == kDeepLinkScheme) {
                MetaVisage::LicenseManager::Instance().QueueDeepLink(msg);
            } else {
                MV_LOG_INFO("SingleInstance: second launch signalled");
            }
            // PR #25 review feedback: any second-launch signal (deep
            // link or plain raise) should bring the running first
            // instance to the foreground.
            MetaVisage::AppWindow::RequestForegroundFocus();
        })) {
        MV_LOG_INFO("MetaVisage already running; exiting second instance");
        return 0;
    }

    // Cold-launch deep-link: a brand-new process started with the URI in
    // argv[1] never reaches the named-pipe forwarding path, so queue
    // directly on the LicenseManager (drained by AppWindow::TickOnce on
    // the first frame).
    if (!deepLink.empty()) {
        MetaVisage::LicenseManager::Instance().QueueDeepLink(deepLink);
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
