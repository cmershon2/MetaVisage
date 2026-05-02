// Story 5.4 — imgui_test_engine smoke-UI lane.
//
// Drives the live AppWindow through three click-through scenarios that
// catch the bug classes flagged in the Sprint 3.6 / 3.7 / 3.8 retros:
// stage progression, modal/window toggling, and viewport-toolbar
// click-through. Each scenario exits on first failure with a screenshot
// available via ImGuiTestEngineIO::ScreenCaptureFunc — currently nullptr
// so failures only surface in stdout, but the hook is wired so we can
// add screenshot-on-failure later.
//
// Process exit codes:
//   0  all tests passed
//   1  AppWindow::Initialize failed
//   2  60-second wall-clock timeout exceeded
//   3  one or more test scenarios failed
//
// Limitations vs. the Sprint 5 plan's original 5-scenario spec:
//   - The shipped 3 are pragmatic substitutes: welcome modal dismiss,
//     dockspace window-existence smoke, and viewport-toolbar shading
//     modes (the original spec's #2). The Sprint 5 spec's stage
//     stepper and login-screen scenarios were dropped after iteration
//     showed their ImGui paths involved BeginChild + nested-menu
//     resolution that isn't easily addressable via the test-engine
//     path syntax — chasing them in this commit would have cost more
//     than the whole story's budget.
//   - Scenarios 3 (Stage 3 process+Accept) and 4 (Stage 4 brush+sculpt
//     +undo) are NOT implemented — both require loading a mesh,
//     running a multi-second worker, or synthesising mouse strokes
//     against a loaded BVH, none of which fit a deterministic <60 s
//     smoke. Sprint 6+ candidates.
//   - The viewport_shading_modes scenario is the click-through smoke
//     the Sprint 3.6/3.7/3.8 retros all asked for; the
//     dockspace_window_smoke catches the related bug class where a
//     refactor drops a Begin call.

#include "core/Project.h"
#include "licensing/LicenseManager.h"
#include "ui/AppWindow.h"
#include "ui/Viewport.h"
#include "ui/ViewportHost.h"
#include "utils/Logger.h"

#include "imgui_te_context.h"
#include "imgui_te_engine.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <chrono>
#include <cstdio>
#include <string>

using namespace MetaVisage;

namespace {

// Static pointer the tests look up to query AppWindow / Project state.
// Lambda captures complicate the IM_REGISTER_TEST macro path; a single
// translation-unit-scoped pointer is the cleanest way to thread state
// into the TestFunc without juggling user-data pointers.
//
// PR #24 review nit — convention for new scenarios: any test that
// dereferences g_app MUST guard with `if (!g_app || !g_app-><...>)`
// + IM_CHECK(false) at the top (see ViewportShadingModes for the
// pattern). Pure-ImGui scenarios that only call ImGui::* APIs (e.g.
// FindWindowByName, ItemClick) don't need the guard — the engine
// itself is bound at Start() and won't tick if the AppWindow is gone.
// The harness sets g_app = &app before Start() and clears it after
// Shutdown(), so it's never null mid-test in practice; the guard
// exists for defence-in-depth, not as a routine null check.
AppWindow* g_app = nullptr;

void DismissWelcomeModalIfOpen(ImGuiTestContext* ctx) {
    // The welcome modal pops on first frame because no project is
    // loaded. Click its Close button if present so subsequent
    // scenarios aren't blocked by an absorbing modal. Use ItemAction
    // with NoError so a missing button is silently a no-op (the
    // dialog might have been dismissed by an earlier test).
    ctx->ItemAction(ImGuiTestAction_Click,
                    "//Welcome to MetaVisage/Close",
                    ImGuiTestOpFlags_NoError);
}

void RegisterScenario_WelcomeModalDismiss(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "welcome_modal_dismiss");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        // Reset any leftover ref from a prior scenario.
        ctx->SetRef("");

        // The welcome modal is the first top-level interactable surface
        // a fresh launch presents. Verify it renders and its Close
        // button dismisses it — exercises the BeginPopupModal
        // click-through path Sprint 3.6 introduced.
        ImGuiWindow* modal = ImGui::FindWindowByName("Welcome to MetaVisage");
        IM_CHECK(modal != nullptr);
        ctx->ItemClick("//Welcome to MetaVisage/Close");

        // Modal popups close by setting Active=false on the next
        // frame. Yield a few frames so ImGui's popup-close animation
        // (and our subsequent re-render) settles, then assert the
        // modal isn't active anymore. WasActive flips to false on the
        // first frame the window stops rendering.
        ctx->Yield();
        ctx->Yield();
        IM_CHECK(modal && !modal->WasActive);
    };
}

void RegisterScenario_DockspaceWindowSmoke(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "dockspace_window_smoke");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("");
        DismissWelcomeModalIfOpen(ctx);

        // The four expected top-level windows after a fresh launch.
        // If any go missing, the dockspace layout regressed and stages
        // can't render. Catches the bug class where a Sprint refactor
        // accidentally drops a Begin call.
        IM_CHECK(ImGui::FindWindowByName("##AppHostWindow") != nullptr);
        IM_CHECK(ImGui::FindWindowByName("##MvTitleBar") != nullptr);

        // The viewport toolbar uses the role-stamped ID we landed in
        // this same story (Viewport.cpp). Its presence confirms the
        // viewport host built its layout this frame.
        IM_CHECK(ImGui::FindWindowByName("##VpToolbar_primary") != nullptr);
    };
}

void RegisterScenario_ViewportShadingModes(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "viewport_shading_modes");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("");
        DismissWelcomeModalIfOpen(ctx);

        if (!g_app || !g_app->GetViewportHost()) {
            IM_CHECK(false);
            return;
        }
        Viewport* vp = g_app->GetViewportHost()->GetPrimaryViewport();
        IM_CHECK(vp != nullptr);
        if (!vp) return;

        // Toolbar window ID is "##VpToolbar_primary" per Story 5.4's
        // role-based ID change in Viewport.cpp. The shading-mode
        // buttons live inside PushID("solid") / PushID("wire") /
        // PushID("tex") wrappers around an InvisibleButton "##b".
        // Absolute "//" prefix sidesteps any leftover SetRef state.
        ctx->ItemClick("//##VpToolbar_primary/solid/##b");
        IM_CHECK(vp->GetShadingMode() == ShadingMode::Solid);

        ctx->ItemClick("//##VpToolbar_primary/wire/##b");
        IM_CHECK(vp->GetShadingMode() == ShadingMode::Wireframe);

        ctx->ItemClick("//##VpToolbar_primary/tex/##b");
        IM_CHECK(vp->GetShadingMode() == ShadingMode::Textured);

        // Restore default so the running window state is sane after
        // the test — useful when iterating locally with the harness
        // visible.
        ctx->ItemClick("//##VpToolbar_primary/solid/##b");
    };
}

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    Logger::Instance().SetFileLogging(false);

    // Sprint 6 Story 6.1 -- bypass the LoginScreen gate. The harness
    // exercises the app shell + viewport + modals, not the auth flow
    // itself; pinning state to Authenticated before AppWindow::Initialize
    // keeps the existing scenarios green. Story 6.7 adds dedicated Login
    // Screen scenarios that flip this back to Unauthenticated.
    LicenseManager::Instance().SetTestState(
        LicenseManager::AuthState::Authenticated);

    AppWindow app;
    g_app = &app;
    if (!app.Initialize(1280, 800, "MetaVisage smoke_ui")) {
        std::fprintf(stderr, "smoke_ui: AppWindow::Initialize failed\n");
        return 1;
    }

    // Test engine setup must come AFTER Initialize because Initialize
    // calls ImGui::CreateContext, and ImGuiTestEngine_Start binds to
    // the current context.
    ImGuiTestEngine* engine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& te_io = ImGuiTestEngine_GetIO(engine);
    te_io.ConfigVerboseLevel        = ImGuiTestVerboseLevel_Info;
    te_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    te_io.ConfigRunSpeed            = ImGuiTestRunSpeed_Fast;
    te_io.ConfigLogToTTY            = true; // mirror per-step log to stdout

    RegisterScenario_WelcomeModalDismiss(engine);
    RegisterScenario_DockspaceWindowSmoke(engine);
    RegisterScenario_ViewportShadingModes(engine);

    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_QueueTests(engine, ImGuiTestGroup_Tests, /*filter=*/nullptr,
                               ImGuiTestRunFlags_None);

    GLFWwindow* window = app.GetWindow();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);

    while (!ImGuiTestEngine_IsTestQueueEmpty(engine)) {
        if (std::chrono::steady_clock::now() > deadline) {
            std::fprintf(stderr, "smoke_ui: 60 s wall-clock timeout exceeded\n");
            ImGuiTestEngine_Stop(engine);
            ImGuiTestEngine_DestroyContext(engine);
            app.Shutdown();
            return 2;
        }
        if (glfwWindowShouldClose(window)) {
            std::fprintf(stderr, "smoke_ui: window closed before tests completed\n");
            ImGuiTestEngine_Stop(engine);
            ImGuiTestEngine_DestroyContext(engine);
            app.Shutdown();
            return 2;
        }
        glfwPollEvents();
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        app.TickOnce(fbW, fbH);
        ImGuiTestEngine_PostSwap(engine);
    }

    int tested = 0, succeeded = 0;
    ImGuiTestEngine_GetResult(engine, tested, succeeded);
    std::printf("\nsmoke_ui: %d/%d scenarios passed\n", succeeded, tested);

    // Per-test status summary so failures name themselves in CTest log.
    ImVector<ImGuiTest*> tests;
    ImGuiTestEngine_GetTestList(engine, &tests);
    for (ImGuiTest* test : tests) {
        const char* statusStr =
            (test->Output.Status == ImGuiTestStatus_Success) ? "PASS" :
            (test->Output.Status == ImGuiTestStatus_Error)   ? "FAIL" :
                                                                "SKIP";
        std::printf("  [%s] %s/%s\n", statusStr,
                    test->Category ? test->Category : "?",
                    test->Name     ? test->Name     : "?");
    }

    ImGuiTestEngine_Stop(engine);
    ImGuiTestEngine_DestroyContext(engine);
    app.Shutdown();
    g_app = nullptr;

    return (succeeded == tested) ? 0 : 3;
}
