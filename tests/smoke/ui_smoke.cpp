// Story 5.4 — imgui_test_engine smoke-UI lane.
//
// Drives the live AppWindow through focused smoke scenarios that catch
// the bug classes flagged across the Sprint 3.6-7 retros: modal/window
// toggling, viewport-toolbar click-through, standalone startup, stage progression,
// and unrestricted brush selection. Each scenario exits on first failure with a screenshot
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
//   - The original three pragmatic substitutes remain: welcome modal
//     dismiss, dockspace window-existence smoke, and viewport-toolbar
//     shading modes, stage progression, and standalone brush selection.
//   - Full loaded-asset Process/Accept and sculpt/undo gestures remain
//     follow-ups. A tiny in-memory mesh now covers worker completion and
//     project replacement without a multi-second production mesh solve.
//   - The viewport_shading_modes scenario is the click-through smoke
//     the Sprint 3.6/3.7/3.8 retros all asked for; the
//     dockspace_window_smoke catches the related bug class where a
//     refactor drops a Begin call.

#include "core/Project.h"
#include "core/Mesh.h"
#include "core/TextureData.h"
#include "core/Types.h"
#include "core/UndoStack.h"
#include "ui/AppWindow.h"
#include "ui/Viewport.h"
#include "ui/ViewportHost.h"
#include "utils/Logger.h"

#include "imgui_te_context.h"
#include "imgui_te_engine.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <fstream>
#include <string>

using namespace MetaVisage;

namespace MetaVisage {

// Drive the worker/project lifecycle without timing-dependent menu clicks.
// The fixture uses the same StartMorph and New handlers as the live UI.
struct AppWindowTestAccess {
    static void StartMorph(AppWindow& app) { app.StartMorph(); }
    static void NewProject(AppWindow& app) { app.OnFileNewProject(); }
    static bool MorphRunning(const AppWindow& app) { return app.morphRunning_.load(); }
    static void ImportTargetMesh(AppWindow& app, const std::string& path) {
        app.ImportTargetMeshFromDisk(path);
    }
    static void RefreshTargetTexturePreview(AppWindow& app) { app.RefreshTargetTexturePreview(); }
};

} // namespace MetaVisage

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

// Test bodies run on the engine's coroutine thread, where the GL context
// is not current. Anything that touches GL (texture/mesh uploads) goes
// through RunOnMainThread, which the harness loop drains before each tick.
std::function<void()> g_mainThreadTask;

void RunOnMainThread(ImGuiTestContext* ctx, std::function<void()> task) {
    g_mainThreadTask = std::move(task);
    while (g_mainThreadTask) ctx->Yield();
}

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
        // the test - useful when iterating locally with the harness
        // visible.
        ctx->ItemClick("//##VpToolbar_primary/solid/##b");
    };
}

void RegisterScenario_StageStepperProgression(ImGuiTestEngine* engine) {
    // Sprint 7 Story 7.6 -- carryover from Sprint 5 retro AI #1 (Stage
    // Stepper portion).
    // Verifies the StageStepper window mounts and tracks Project::
    // SetCurrentStage transitions across the four-stage workflow. Forward-
    // gating logic (CanProceedToNextStage) requires per-stage prereqs --
    // mesh loaded, points placed, morph processed -- that a hermetic
    // harness cannot satisfy without a fixture. This tests UI state
    // transitions directly.
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "stage_stepper_progression");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("");
        DismissWelcomeModalIfOpen(ctx);

        if (!g_app || !g_app->GetProject()) {
            IM_CHECK(false);
            return;
        }
        Project* project = g_app->GetProject();

        // ImGui's BeginChild adds a synthetic hash suffix to the child
        // window's stored name (parent_path/child_name_<id-hex>), so a
        // direct FindWindowByName("##MvStageStepper") returns nullptr.
        // The host window's existence is the proof the Stepper rendered
        // -- StageStepper::Render is called unconditionally inside the
        // host's Begin scope every frame. The transition assertions
        // below are the actual contract this scenario verifies.
        IM_CHECK(ImGui::FindWindowByName("##AppHostWindow") != nullptr);

        const WorkflowStage stages[] = {
            WorkflowStage::Alignment,
            WorkflowStage::PointReference,
            WorkflowStage::Morph,
            WorkflowStage::TouchUp,
        };
        for (WorkflowStage stage : stages) {
            project->SetCurrentStage(stage);
            ctx->Yield();
            ctx->Yield();
            IM_CHECK(project->GetCurrentStage() == stage);
            // Host still active -- if anything in the per-frame render
            // path crashes (StageStepper included), the host stops
            // rendering and this fires.
            ImGuiWindow* host = ImGui::FindWindowByName("##AppHostWindow");
            IM_CHECK(host != nullptr && host->WasActive);
        }

        // Restore Stage 1 so any subsequent manual exploration of the
        // harness window starts from a coherent state.
        project->SetCurrentStage(WorkflowStage::Alignment);
        ctx->Yield();
    };
}

void RegisterScenario_StandaloneBrushes(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "standalone_brushes");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("");
        DismissWelcomeModalIfOpen(ctx);
        IM_CHECK(g_app != nullptr && g_app->GetProject() != nullptr);
        ImGuiWindow* host = ImGui::FindWindowByName("##AppHostWindow");
        IM_CHECK(host != nullptr && host->WasActive);
        Project* project = g_app->GetProject();
        project->SetCurrentStage(WorkflowStage::TouchUp);
        Viewport* viewport = g_app->GetViewportHost()->GetPrimaryViewport();
        IM_CHECK(viewport != nullptr);
        int brushKey = GLFW_KEY_1;
        for (BrushType brush : {BrushType::Smooth, BrushType::Grab,
                                BrushType::PushPull, BrushType::Inflate}) {
            viewport->SetBrushType(brush);
            IM_CHECK(viewport->GetBrushType() == brush);
            viewport->OnKey(brushKey++, 0, GLFW_PRESS, 0);
            ctx->Yield();
            ctx->Yield();
            IM_CHECK(project->GetSculptState().activeBrush == brush);
            IM_CHECK(viewport->GetBrushType() == brush);
        }
        project->SetCurrentStage(WorkflowStage::Alignment);
        ctx->Yield();
    };
}

void RegisterScenario_ProjectSwitchDuringMorph(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "project_switch_during_morph");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        if (!g_app || !g_app->GetProject()) { IM_CHECK(false); return; }
        DismissWelcomeModalIfOpen(ctx);

        const auto prepare = [] {
            auto mesh = std::make_shared<Mesh>();
            mesh->SetVertices({Vector3(0, 0, 0), Vector3(1, 0, 0),
                               Vector3(0, 1, 0), Vector3(0, 0, 1)});
            std::vector<Face> faces(4);
            faces[0].vertexIndices = {0, 2, 1};
            faces[1].vertexIndices = {0, 1, 3};
            faces[2].vertexIndices = {0, 3, 2};
            faces[3].vertexIndices = {1, 2, 3};
            mesh->SetFaces(faces);
            mesh->CalculateNormals();
            Project& project = *g_app->GetProject();
            for (MeshReference* ref : {&project.GetMorphMesh(), &project.GetTargetMesh()}) {
                ref->mesh = mesh;
                ref->isLoaded = true;
            }
            auto& morph = project.GetMorphData();
            morph.nricpEnableBoundaryExclusion = false;
            morph.nricpStiffnessSteps = 1;
            morph.nricpIcpIterations = 1;
            morph.nricpOptimizationIterations = 1;
            morph.nricpSamplingInitial = 0;
            morph.nricpSamplingFinal = 0;
            for (int i = 0; i < 4; ++i) {
                PointCorrespondence point;
                point.pointID = i;
                point.morphMeshVertexIndex = point.targetMeshVertexIndex = i;
                point.morphMeshPosition = point.targetMeshPosition = mesh->GetVertices()[i];
                project.GetPointReferenceData().correspondences.push_back(point);
            }
        };

        // First verify a result still applies to the workspace that started it.
        AppWindowTestAccess::NewProject(*g_app);
        prepare();
        AppWindowTestAccess::StartMorph(*g_app);
        while (AppWindowTestAccess::MorphRunning(*g_app)) ctx->Yield();
        IM_CHECK(g_app->GetProject()->GetMorphData().isProcessed);
        IM_CHECK(g_app->GetProject()->GetMorphData().deformedMorphMesh != nullptr);

        AppWindowTestAccess::NewProject(*g_app);
        prepare();
        AppWindowTestAccess::StartMorph(*g_app);
        // No yield: replace the workspace before completion can be consumed,
        // even if this tiny solver has already finished on the worker thread.
        AppWindowTestAccess::NewProject(*g_app);
        while (AppWindowTestAccess::MorphRunning(*g_app)) ctx->Yield();
        IM_CHECK(!g_app->GetProject()->GetMorphData().isProcessed);
        IM_CHECK(g_app->GetProject()->GetMorphData().deformedMorphMesh == nullptr);
        IM_CHECK(!g_app->GetProject()->GetMorphMesh().isLoaded);
    };
}

// Sprint 9 — shared fixture: a fresh project in Alignment with a unit
// tetrahedron as the target mesh, framed by the primary camera.
Viewport* PrepareAlignmentFixture(ImGuiTestContext* ctx) {
    AppWindowTestAccess::NewProject(*g_app);
    auto mesh = std::make_shared<Mesh>();
    mesh->SetVertices({Vector3(0, 0, 0), Vector3(1, 0, 0),
                       Vector3(0, 1, 0), Vector3(0, 0, 1)});
    std::vector<Face> faces(4);
    faces[0].vertexIndices = {0, 2, 1};
    faces[1].vertexIndices = {0, 1, 3};
    faces[2].vertexIndices = {0, 3, 2};
    faces[3].vertexIndices = {1, 2, 3};
    mesh->SetFaces(faces);
    mesh->CalculateNormals();
    Project& project = *g_app->GetProject();
    project.GetTargetMesh().mesh = mesh;
    project.GetTargetMesh().isLoaded = true;
    project.SetCurrentStage(WorkflowStage::Alignment);
    Viewport* vp = g_app->GetViewportHost()->GetPrimaryViewport();
    vp->GetCamera()->FocusOnBounds(mesh->GetBounds());
    ctx->Yield(3); // let the viewport record its pixel size
    return vp;
}

// Projects a world point to viewport-local pixels (Y down), matching the
// space Viewport::OnMouse* receives.
Vector3 WorldToViewportPixels(const Viewport& vp, const Vector3& p) {
    const float w = vp.GetPixelWidth();
    const float h = vp.GetPixelHeight();
    const Camera& cam = *const_cast<Viewport&>(vp).GetCamera();
    const Matrix4x4 vpm = cam.GetProjectionMatrix(w / h) * cam.GetViewMatrix();
    const float* m = vpm.m;
    const float cx = m[0] * p.x + m[4] * p.y + m[8]  * p.z + m[12];
    const float cy = m[1] * p.x + m[5] * p.y + m[9]  * p.z + m[13];
    const float cw = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
    return Vector3((cx / cw + 1.0f) * 0.5f * w, (1.0f - cy / cw) * 0.5f * h, 0.0f);
}

void RegisterScenario_AlignmentTransformHotkeys(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "alignment_transform_hotkeys");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        if (!g_app || !g_app->GetProject() || !g_app->GetViewportHost()) { IM_CHECK(false); return; }
        DismissWelcomeModalIfOpen(ctx);
        Viewport* vp = PrepareAlignmentFixture(ctx);
        IM_CHECK(vp != nullptr);
        if (!vp) return;

        // Blender-style bindings, which the tool rail badges now advertise.
        const struct { int key; TransformMode mode; } kBindings[] = {
            {GLFW_KEY_G, TransformMode::Move},
            {GLFW_KEY_R, TransformMode::Rotate},
            {GLFW_KEY_S, TransformMode::Scale},
        };
        for (const auto& b : kBindings) {
            vp->OnKey(b.key, 0, GLFW_PRESS, 0);
            ctx->Yield();
            IM_CHECK(vp->GetTransformMode() == b.mode);
        }
        AppWindowTestAccess::NewProject(*g_app);
    };
}

void RegisterScenario_AlignmentGizmoTracksCursor(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "alignment_gizmo_tracks_cursor");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        if (!g_app || !g_app->GetProject() || !g_app->GetViewportHost()) { IM_CHECK(false); return; }
        DismissWelcomeModalIfOpen(ctx);
        Viewport* vp = PrepareAlignmentFixture(ctx);
        IM_CHECK(vp != nullptr && vp->GetPixelHeight() > 0.0f);
        if (!vp || vp->GetPixelHeight() <= 0.0f) return;
        Project& project = *g_app->GetProject();
        Transform& xform = project.GetTargetMesh().transform;
        UndoStack* undo = g_app->GetUndoStack();
        IM_CHECK(undo != nullptr);
        if (!undo) return;

        // Drag the gizmo 120 px right / 40 px up: the mesh centre must land
        // under the cursor (the old fixed 0.01 units/px did not track).
        const Mesh& mesh = *project.GetTargetMesh().mesh;
        auto centreOnScreen = [&] {
            const Vector3 c = mesh.GetBounds().Center() + xform.GetPosition();
            return WorldToViewportPixels(*vp, c);
        };
        vp->SetTransformMode(TransformMode::Move);
        const int undoBefore = undo->Size();
        const Vector3 start = centreOnScreen();
        vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0, start.x, start.y);
        vp->OnMouseMove(start.x + 60.0f, start.y - 20.0f);
        vp->OnMouseMove(start.x + 120.0f, start.y - 40.0f);
        vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0,
                          start.x + 120.0f, start.y - 40.0f);
        ctx->Yield();
        const Vector3 end = centreOnScreen();
        IM_CHECK_LE(std::abs(end.x - (start.x + 120.0f)), 1.0f);
        IM_CHECK_LE(std::abs(end.y - (start.y - 40.0f)), 1.0f);
        IM_CHECK_EQ(undo->Size(), undoBefore + 1); // one undo per drag

        // X-constrained drag in the front view moves along X only.
        const Vector3 before = xform.GetPosition();
        vp->OnKey(GLFW_KEY_X, 0, GLFW_PRESS, 0);
        IM_CHECK(vp->GetAxisConstraint() == AxisConstraint::X);
        const Vector3 s2 = centreOnScreen();
        vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0, s2.x, s2.y);
        vp->OnMouseMove(s2.x + 80.0f, s2.y + 30.0f);
        vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0, s2.x + 80.0f, s2.y + 30.0f);
        ctx->Yield();
        const Vector3 after = xform.GetPosition();
        IM_CHECK_GT(after.x, before.x);
        IM_CHECK_EQ(after.y, before.y);
        IM_CHECK_EQ(after.z, before.z);
        IM_CHECK_LE(std::abs(centreOnScreen().x - (s2.x + 80.0f)), 1.0f);
        AppWindowTestAccess::NewProject(*g_app);
    };
}

void RegisterScenario_AlignmentGizmoAllViews(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "alignment_gizmo_all_views");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        if (!g_app || !g_app->GetProject() || !g_app->GetViewportHost()) { IM_CHECK(false); return; }
        DismissWelcomeModalIfOpen(ctx);

        // Orbited perspective plus the numpad orthographic views. Each
        // names an axis lying in its view plane for the constrained drag.
        const struct { const char* name; int key; int axisKey; int axis; } kViews[] = {
            {"orbited",   0,              GLFW_KEY_X, 0},
            {"front (1)", GLFW_KEY_KP_1,  GLFW_KEY_X, 0},
            {"right (3)", GLFW_KEY_KP_3,  GLFW_KEY_Z, 2},
            {"top (7)",   GLFW_KEY_KP_7,  GLFW_KEY_X, 0},
        };
        for (const auto& v : kViews) {
            ctx->LogInfo("view: %s", v.name);
            Viewport* vp = PrepareAlignmentFixture(ctx);
            if (!vp || vp->GetPixelHeight() <= 0.0f) { IM_CHECK(false); return; }
            if (v.key) vp->OnKey(v.key, 0, GLFW_PRESS, 0);
            else vp->GetCamera()->Orbit(60.0f, 35.0f);
            ctx->Yield();

            Transform& xform = g_app->GetProject()->GetTargetMesh().transform;
            const Mesh& mesh = *g_app->GetProject()->GetTargetMesh().mesh;
            auto centreOnScreen = [&] {
                return WorldToViewportPixels(*vp, mesh.GetBounds().Center() + xform.GetPosition());
            };

            // Free drag keeps the mesh centre under the cursor.
            vp->SetTransformMode(TransformMode::Move);
            const Vector3 s1 = centreOnScreen();
            vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0, s1.x, s1.y);
            vp->OnMouseMove(s1.x - 70.0f, s1.y + 50.0f);
            vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0, s1.x - 70.0f, s1.y + 50.0f);
            ctx->Yield();
            const Vector3 e1 = centreOnScreen();
            IM_CHECK_LE(std::abs(e1.x - (s1.x - 70.0f)), 1.0f);
            IM_CHECK_LE(std::abs(e1.y - (s1.y + 50.0f)), 1.0f);

            // Axis-constrained drag changes only that axis.
            const Vector3 before = xform.GetPosition();
            vp->SetTransformMode(TransformMode::Move);
            vp->OnKey(v.axisKey, 0, GLFW_PRESS, 0);
            const Vector3 s2 = centreOnScreen();
            vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0, s2.x, s2.y);
            vp->OnMouseMove(s2.x + 90.0f, s2.y - 25.0f);
            vp->OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0, s2.x + 90.0f, s2.y - 25.0f);
            ctx->Yield();
            const Vector3 after = xform.GetPosition();
            const float b[3] = {before.x, before.y, before.z};
            const float a[3] = {after.x, after.y, after.z};
            for (int i = 0; i < 3; ++i) {
                if (i == v.axis) IM_CHECK_NE(a[i], b[i]);
                else IM_CHECK_EQ(a[i], b[i]);
            }
        }
        AppWindowTestAccess::NewProject(*g_app);
    };
}

void RegisterScenario_AlignmentTextureSurvivesTargetSwap(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "alignment_texture_survives_target_swap");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        if (!g_app || !g_app->GetProject() || !g_app->GetViewportHost()) { IM_CHECK(false); return; }
        DismissWelcomeModalIfOpen(ctx);
        Viewport* vp = PrepareAlignmentFixture(ctx);
        if (!vp) { IM_CHECK(false); return; }
        Project& project = *g_app->GetProject();

        // Albedo on the first target, as ImportTextureForSlot leaves it.
        auto albedo = TextureData::Create(2, 2);
        project.GetTargetTextures().Add("Albedo", albedo);
        RunOnMainThread(ctx, [] { AppWindowTestAccess::RefreshTargetTexturePreview(*g_app); });
        const Mesh* oldMesh = project.GetTargetMesh().mesh.get();
        IM_CHECK(vp->HasMeshTexture(oldMesh));

        // Replace the target from disk; the project's albedo must follow.
        const std::filesystem::path obj =
            std::filesystem::temp_directory_path() / "metavisage_smoke_target_swap.obj";
        {
            std::ofstream out(obj);
            out << "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 0 0 1\n"
                   "f 1 3 2\nf 1 2 4\nf 1 4 3\nf 2 3 4\n";
        }
        RunOnMainThread(ctx, [&] { AppWindowTestAccess::ImportTargetMesh(*g_app, obj.string()); });
        const Mesh* newMesh = project.GetTargetMesh().mesh.get();
        IM_CHECK(project.GetTargetMesh().isLoaded);
        IM_CHECK(newMesh != nullptr && newMesh != oldMesh);
        IM_CHECK(vp->HasMeshTexture(newMesh));
        std::error_code ec;
        std::filesystem::remove(obj, ec);
        AppWindowTestAccess::NewProject(*g_app);
    };
}

void RegisterScenario_AlignmentTypedTransformEntry(ImGuiTestEngine* engine) {
    ImGuiTest* t = IM_REGISTER_TEST(engine, "smoke", "alignment_typed_transform_entry");
    t->TestFunc = [](ImGuiTestContext* ctx) {
        if (!g_app || !g_app->GetProject() || !g_app->GetUndoStack()) { IM_CHECK(false); return; }
        DismissWelcomeModalIfOpen(ctx);
        PrepareAlignmentFixture(ctx);
        Transform& xform = g_app->GetProject()->GetTargetMesh().transform;
        UndoStack* undo = g_app->GetUndoStack();

        // Child window paths are mangled; resolve the sidebar body first.
        ctx->SetRef(ctx->WindowInfo("//Stage/##stage1body").Window);
        const char* kPosX = "Position/##xform/X/##axisDrag";

        // Double-click → type → Enter commits once, with exactly one undo.
        const int undoBefore = undo->Size();
        ctx->ItemDoubleClick(kPosX);
        ctx->Yield(2);
        ctx->KeyCharsReplaceEnter("12.5");
        ctx->Yield(2);
        IM_CHECK_FLOAT_NEAR(xform.GetPosition().x, 12.5f, 1e-4f);
        IM_CHECK_EQ(undo->Size(), undoBefore + 1);

        // Esc cancels: value and undo stack unchanged.
        ctx->ItemDoubleClick(kPosX);
        ctx->Yield(2);
        ctx->KeyCharsReplace("99");
        ctx->KeyPress(ImGuiKey_Escape);
        ctx->Yield(2);
        IM_CHECK_FLOAT_NEAR(xform.GetPosition().x, 12.5f, 1e-4f);
        IM_CHECK_EQ(undo->Size(), undoBefore + 1);

        ctx->SetRef("");
        AppWindowTestAccess::NewProject(*g_app);
    };
}

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    Logger::Instance().SetFileLogging(false);

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
    RegisterScenario_StageStepperProgression(engine);
    RegisterScenario_StandaloneBrushes(engine);
    RegisterScenario_ProjectSwitchDuringMorph(engine);
    RegisterScenario_AlignmentTransformHotkeys(engine);
    RegisterScenario_AlignmentGizmoTracksCursor(engine);
    RegisterScenario_AlignmentGizmoAllViews(engine);
    RegisterScenario_AlignmentTextureSurvivesTargetSwap(engine);
    RegisterScenario_AlignmentTypedTransformEntry(engine);

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
        if (g_mainThreadTask) {
            g_mainThreadTask();
            g_mainThreadTask = nullptr;
        }
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
