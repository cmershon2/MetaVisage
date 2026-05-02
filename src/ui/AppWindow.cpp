// NOMINMAX must land before any header that could transitively include
// <windows.h> — otherwise its `min` / `max` macros shadow std::min /
// std::max wherever AppWindow.cpp uses them (e.g. the sculpt-stroke undo
// builder). Hoisted here even before the project headers because GLFW's
// imgui_impl_* and the Win32 native helper both pull windows.h in.
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#endif

#include "ui/AppWindow.h"
#include "ui/FileDialogs.h"
#include "ui/LoginScreen.h"
#include "ui/MvStyle.h"
#include "ui/StageStepper.h"
#include "ui/ToolRail.h"
#include "ui/Viewport.h"
#include "ui/ViewportHost.h"
#include "core/Camera.h"
#include "core/Mesh.h"
#include "core/Project.h"
#include "core/TextureData.h"
#include "core/UndoActions.h"
#include "core/UndoStack.h"
#include "core/Types.h"
#include "deformation/MeshDeformer.h"
#include "deformation/NRICPDeformer.h"
#include "io/ProjectSerializer.h"
#include "io/MeshExporter.h"
#include "io/PresetStore.h"
#include "licensing/LicenseManager.h"
#include "baking/TextureBaker.h"
#include "baking/TextureWriter.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "utils/OpenUrl.h"
#include "utils/Paths.h"
#include "utils/RayCaster.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <cstdio>
#include <filesystem>
#include <format>
#include <string>

// Sprint 3.8 follow-up — frameless OS window + custom title bar. We expose
// the Win32 HWND so our WM_NCHITTEST handler can tell Windows where the
// drag-able title bar / resize edges are, which buys us back native Aero
// Snap, edge-resize cursors, and the right-click system menu without the
// double title bar redundancy of the previous decorated-window approach.
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
// NOMINMAX / WIN32_LEAN_AND_MEAN are defined at the very top of this file
// (before any header that could transitively pull windows.h in).
#include <windows.h>
#include <windowsx.h>
#endif

namespace MetaVisage {

namespace {

void GlfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
    MV_LOG_ERROR(std::format("GLFW error {}: {}", error, description));
}

#ifdef _WIN32
// =============================================================================
// Frameless-window hit-test hook
//
// GLFW's GLFW_DECORATED=false strips the OS chrome but also strips the
// native edge-resize / title-bar-drag / Aero Snap behaviour. We get those
// back by subclassing the GLFW window's WndProc and answering the OS's
// WM_NCHITTEST query: "given this cursor position, which non-client zone
// is it in?"
//
// Layout we tell Windows about:
//
//   ┌─────────────────────────────────────────────────────────┐
//   │ ▲ resize-top                            ▲ caption btns ▲│
//   │ ◀ resize-left  HTCAPTION  (drag-zone)   ▶  HTCLIENT     │
//   ├─────────────────────────────────────────────────────────┤  ← y = TitleBarH
//   │                                                         │
//   │                  HTCLIENT  (ImGui)                      │
//   │                                                         │
//   └─────────────────────────────────────────────────────────┘
//
// HTCAPTION makes the OS handle drag, double-click-maximize, drag-to-snap.
// HTCLIENT lets ImGui's invisible buttons receive the click for our
// custom min/max/close caption buttons. The 4 px resize border on each
// edge gives the user the standard resize cursor + drag.
// =============================================================================

// Constants must match RenderTitleBar's layout in this same file.
constexpr int kTitleBarH        = 36;
constexpr int kCaptionBtnW      = 46;
constexpr int kCaptionClusterW  = 3 * kCaptionBtnW;   // min + max + close
constexpr int kResizeBorder     = 4;

WNDPROC g_originalWndProc = nullptr;

LRESULT CALLBACK MvFramelessWndProc(HWND hwnd, UINT msg,
                                    WPARAM wp, LPARAM lp) {
    switch (msg) {
        // Tell Windows the entire window rect is client area — no
        // automatic non-client frame inset that would otherwise put a
        // few-pixel border between the actual window edge and our
        // GL/ImGui surface.
        case WM_NCCALCSIZE:
            if (wp == TRUE) return 0;
            break;

        // Pre-empt the default DWM rendering of a fake border on
        // top of borderless windows by reporting size changes early.
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            ::ScreenToClient(hwnd, &pt);

            RECT rc;
            ::GetClientRect(hwnd, &rc);
            const int cw = rc.right - rc.left;
            const int ch = rc.bottom - rc.top;

            const bool maximized = ::IsZoomed(hwnd);
            // Resize edges are suppressed when maximized — the window is
            // already pinned to the work area, and a stray edge-drag
            // would un-maximize at the cursor.
            if (!maximized) {
                const bool top    = pt.y < kResizeBorder;
                const bool bottom = pt.y > ch - kResizeBorder;
                const bool left   = pt.x < kResizeBorder;
                const bool right  = pt.x > cw - kResizeBorder;
                if (top    && left)  return HTTOPLEFT;
                if (top    && right) return HTTOPRIGHT;
                if (bottom && left)  return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;
                if (top)    return HTTOP;
                if (bottom) return HTBOTTOM;
                if (left)   return HTLEFT;
                if (right)  return HTRIGHT;
            }

            // Custom title bar zone.
            if (pt.y < kTitleBarH) {
                // The right-most cluster is our min/max/close caption
                // buttons; pass clicks through to ImGui so InvisibleButton
                // can fire its glfwIconify / Maximize / SetWindowShouldClose
                // callbacks.
                if (pt.x > cw - kCaptionClusterW) {
                    return HTCLIENT;
                }
                return HTCAPTION;
            }

            return HTCLIENT;
        }
    }
    return ::CallWindowProc(g_originalWndProc, hwnd, msg, wp, lp);
}

void InstallFramelessHook(GLFWwindow* glfwWindow) {
    HWND hwnd = ::glfwGetWin32Window(glfwWindow);
    if (!hwnd) {
        MV_LOG_WARNING("InstallFramelessHook: glfwGetWin32Window returned null");
        return;
    }
    g_originalWndProc = reinterpret_cast<WNDPROC>(
        ::SetWindowLongPtr(hwnd, GWLP_WNDPROC,
                           reinterpret_cast<LONG_PTR>(MvFramelessWndProc)));
    // Force the OS to recompute the non-client area now that we own
    // WM_NCCALCSIZE — without this nudge the pre-existing frame inset
    // sticks until the user resizes.
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                   SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                   SWP_NOZORDER | SWP_NOACTIVATE);
}
#endif // _WIN32

// Case-insensitive suffix check used in path-extension comparisons.
bool EndsWithICase(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[s.size() - suffix.size() + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i]))) {
            return false;
        }
    }
    return true;
}

// Filename portion of a path (no directory).
std::string FileName(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

const char* StageDisplayName(WorkflowStage s) {
    switch (s) {
        case WorkflowStage::Alignment:      return "Alignment";
        case WorkflowStage::PointReference: return "Point Reference";
        case WorkflowStage::Morph:          return "Morph";
        case WorkflowStage::TouchUp:        return "Touch Up";
    }
    return "";
}

std::vector<FileDialogs::Filter> ProjectFilters() {
    return {{"MetaVisage Project", "mmproj"}};
}

std::vector<FileDialogs::Filter> MeshImportFilters() {
    return {
        {"3D mesh (FBX, OBJ, GLTF)", "fbx,obj,gltf,glb"},
        {"FBX", "fbx"},
        {"OBJ", "obj"},
        {"GLTF", "gltf,glb"},
    };
}

std::vector<FileDialogs::Filter> ExportFiltersFor(ExportFormat format) {
    switch (format) {
        case ExportFormat::FBX:  return {{"FBX", "fbx"}};
        case ExportFormat::OBJ:  return {{"OBJ", "obj"}};
        case ExportFormat::GLTF: return {{"GLTF", "gltf"}};
    }
    return {{"FBX", "fbx"}};
}

} // namespace

AppWindow::AppWindow() = default;

AppWindow::~AppWindow() {
    Shutdown();
}

bool AppWindow::Initialize(int width, int height, const char* title) {
    glfwSetErrorCallback(&GlfwErrorCallback);

    if (!glfwInit()) {
        MV_LOG_CRITICAL("glfwInit failed");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA
    // Sprint 3.8 follow-up — frameless mode. Sprint 3.8.2's plan kept the
    // OS native title bar and added our custom one inside the client area,
    // which produced two title bars stacked. With DECORATED=false the OS
    // chrome disappears entirely; the InstallFramelessHook below restores
    // native drag / resize / Aero Snap by answering WM_NCHITTEST.
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        MV_LOG_CRITICAL("glfwCreateWindow failed");
        glfwTerminate();
        return false;
    }

#ifdef _WIN32
    InstallFramelessHook(window_);
#endif

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        MV_LOG_CRITICAL(std::format("glewInit failed: {}",
            reinterpret_cast<const char*>(glewGetErrorString(glewStatus))));
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imguiContextCreated_ = true;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr; // don't persist imgui.ini in cwd

    // Sprint 3.8.1 — load TTF fonts (Inter / Saira / JetBrains Mono) into the
    // atlas before the GL backend uploads it during ImGui_ImplOpenGL3_Init,
    // then apply the design-token color palette. Missing TTFs are non-fatal:
    // MvStyle::LoadFonts falls back to ProggyClean and logs a warning so a
    // dev who pulls the repo without assets/fonts/ still gets a usable build.
    MvStyle::LoadFonts();
    MvStyle::Apply();

    // install_callbacks=false — we install our own GLFW callbacks below and
    // forward to ImGui_ImplGlfw_* inside them so we can gate viewport input on
    // ImGui's WantCapture flags without fighting the backend's chain logic.
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, false)) {
        MV_LOG_CRITICAL("ImGui_ImplGlfw_InitForOpenGL failed");
        Shutdown();
        return false;
    }
    imguiGlfwInitialized_ = true;

    if (!ImGui_ImplOpenGL3_Init("#version 430 core")) {
        MV_LOG_CRITICAL("ImGui_ImplOpenGL3_Init failed");
        Shutdown();
        return false;
    }
    imguiOpenGLInitialized_ = true;

    // PNG / GL-texture assets — must run after ImGui_ImplOpenGL3_Init so the
    // GL context is current and the logo upload succeeds.
    MvStyle::LoadAssets();

    {
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        gpuRenderer_ = renderer ? renderer : "GPU";
    }
    MV_LOG_INFO(std::format("AppWindow initialized: {}x{}, GL {}, GLSL {}, GPU {}",
        width, height,
        reinterpret_cast<const char*>(glGetString(GL_VERSION)),
        reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)),
        gpuRenderer_));

    viewportHost_ = std::make_unique<ViewportHost>();
    if (!viewportHost_->Initialize()) {
        MV_LOG_CRITICAL("ViewportHost initialization failed");
        viewportHost_.reset();
        Shutdown();
        return false;
    }

    project_ = std::make_unique<Project>();
    // Quick Wrap is the canonical project default per CLAUDE.md and the
    // Sprint 4 plan — apply on every fresh-project construction so the
    // inspector card row opens with that built-in highlighted.
    WrapPreset::QuickWrap().ApplyTo(project_->GetMorphData());
    undoStack_ = std::make_unique<UndoStack>();
    viewportHost_->SetProject(project_.get());

    // Sprint 6 Story 6.1 -- bootstrap the auth state machine off the
    // encrypted token cache. Runs after Config is loaded (main.cpp:13)
    // and before the LoginScreen first renders, so the cached-session
    // fast path can skip straight into Authenticated and the user never
    // sees the sign-in surface flash.
    LicenseManager::Instance().Initialize();

    userPresets_ = PresetStore::LoadAll();
    statusMessage_ = "Ready";

    // Stage 2 point placement: both viewports fire OnPointPlaced when the user
    // LMB-clicks a mesh under the ImGui/Point Reference stage gate. The right
    // viewport only exists in dual mode (Stage 2), but we wire both up front
    // so a callback is always in place. Port of MainWindow::OnPointPlaced.
    auto onPointPlaced = [this](PointSide side, Vector3 pos, int vertexIndex) {
        if (!project_) return;
        auto& data = project_->GetPointReferenceData();
        auto& correspondences = data.correspondences;

        // Snapshot for undo before mutation. The whole correspondences vector
        // is small (typically <50 entries), so a copy is cheaper than tracking
        // diffs across the symmetry-fuse / pair-creation paths below.
        const std::vector<PointCorrespondence> beforeSnapshot = correspondences;

        int nextID = 1;
        for (const auto& c : correspondences) {
            if (c.pointID >= nextID) nextID = c.pointID + 1;
        }

        int modifiedIndex = -1;
        // Reuse a pending partial correspondence on the opposite side so the
        // two halves fuse into a single PointCorrespondence, matching v1.0.
        for (int i = 0; i < static_cast<int>(correspondences.size()); ++i) {
            auto& c = correspondences[i];
            if (side == PointSide::Target &&
                c.targetMeshVertexIndex < 0 && c.morphMeshVertexIndex >= 0) {
                c.targetMeshPosition = pos;
                c.targetMeshVertexIndex = vertexIndex;
                modifiedIndex = i;
                break;
            }
            if (side == PointSide::Morph &&
                c.morphMeshVertexIndex < 0 && c.targetMeshVertexIndex >= 0) {
                c.morphMeshPosition = pos;
                c.morphMeshVertexIndex = vertexIndex;
                modifiedIndex = i;
                break;
            }
        }

        if (modifiedIndex < 0) {
            PointCorrespondence pc;
            pc.pointID = nextID;
            if (side == PointSide::Target) {
                pc.targetMeshPosition = pos;
                pc.targetMeshVertexIndex = vertexIndex;
            } else {
                pc.morphMeshPosition = pos;
                pc.morphMeshVertexIndex = vertexIndex;
            }
            correspondences.push_back(pc);
            modifiedIndex = static_cast<int>(correspondences.size()) - 1;
        }

        // Symmetry mirror (port of MainWindow::PlaceSymmetricPoint). Mirrors
        // across the configured axis, snaps to the nearest vertex, and fuses
        // with any existing paired point rather than duplicating it.
        if (data.symmetryEnabled) {
            Vector3 mirrored = pos;
            switch (data.symmetryAxis) {
                case Axis::X: mirrored.x = -mirrored.x; break;
                case Axis::Y: mirrored.y = -mirrored.y; break;
                case Axis::Z: mirrored.z = -mirrored.z; break;
            }
            const Mesh* mesh = nullptr;
            const Transform* tf = nullptr;
            if (side == PointSide::Target && project_->GetTargetMesh().mesh) {
                mesh = project_->GetTargetMesh().mesh.get();
                tf   = &project_->GetTargetMesh().transform;
            } else if (side == PointSide::Morph && project_->GetMorphMesh().mesh) {
                mesh = project_->GetMorphMesh().mesh.get();
                tf   = &project_->GetMorphMesh().transform;
            }
            int mirroredVertexIdx = (mesh && tf)
                ? RayCaster::FindNearestVertex(mirrored, *mesh, *tf)
                : -1;
            if (mirroredVertexIdx >= 0) {
                auto& parent = correspondences[modifiedIndex];

                bool filledExisting = false;
                if (parent.isSymmetric && parent.symmetricPairID >= 0) {
                    for (auto& corr : correspondences) {
                        if (corr.pointID == parent.symmetricPairID) {
                            if (side == PointSide::Target) {
                                corr.targetMeshPosition    = mirrored;
                                corr.targetMeshVertexIndex = mirroredVertexIdx;
                            } else {
                                corr.morphMeshPosition    = mirrored;
                                corr.morphMeshVertexIndex = mirroredVertexIdx;
                            }
                            filledExisting = true;
                            break;
                        }
                    }
                }
                if (!filledExisting) {
                    int symID = 1;
                    for (const auto& c : correspondences) {
                        if (c.pointID >= symID) symID = c.pointID + 1;
                    }
                    PointCorrespondence sym;
                    sym.pointID         = symID;
                    sym.isSymmetric     = true;
                    sym.symmetricPairID = parent.pointID;
                    if (side == PointSide::Target) {
                        sym.targetMeshPosition    = mirrored;
                        sym.targetMeshVertexIndex = mirroredVertexIdx;
                    } else {
                        sym.morphMeshPosition    = mirrored;
                        sym.morphMeshVertexIndex = mirroredVertexIdx;
                    }
                    correspondences.push_back(sym);
                    correspondences[modifiedIndex].isSymmetric     = true;
                    correspondences[modifiedIndex].symmetricPairID = sym.pointID;
                }
            }
        }

        // Point placement always mutates (appends or fuses), so push without a
        // before/after equality check (PointCorrespondence has no operator==).
        if (undoStack_) {
            undoStack_->Push(std::make_unique<PointPlacementUndoAction>(
                project_.get(), beforeSnapshot, correspondences, "Place Point"));
        }
    };

    // Stage 2 delete: Delete key on the active viewport fires this. Removes
    // the currently-selected correspondence and its symmetric pair if any.
    // Index-aware deletion helper — used by both the Delete-key path (which
    // reads the currently-selected index from the viewport) and the
    // sidebar's per-row X click (which passes its row index directly).
    deletePointAt_ = [this](int index) {
        if (!project_ || !viewportHost_) return;
        auto& correspondences = project_->GetPointReferenceData().correspondences;
        if (index < 0 || index >= static_cast<int>(correspondences.size())) return;

        const std::vector<PointCorrespondence> beforeSnapshot = correspondences;

        int pairID = -1;
        if (correspondences[index].isSymmetric) {
            pairID = correspondences[index].symmetricPairID;
        }
        correspondences.erase(correspondences.begin() + index);
        if (pairID >= 0) {
            for (auto it = correspondences.begin(); it != correspondences.end(); ++it) {
                if (it->pointID == pairID) { correspondences.erase(it); break; }
            }
        }
        viewportHost_->SetSelectedPointIndex(-1);

        if (undoStack_) {
            undoStack_->Push(std::make_unique<PointPlacementUndoAction>(
                project_.get(), beforeSnapshot, correspondences, "Delete Point"));
        }
    };

    // Delete-key path — figure out which side has the selection and forward
    // to the index-aware helper.
    auto onPointDelete = [this]() {
        if (!viewportHost_) return;
        auto* vp = viewportHost_->GetPrimaryViewport();
        int primaryIdx = vp ? vp->GetSelectedPointIndex() : -1;
        int secondaryIdx = viewportHost_->GetSecondaryViewport()
            ? viewportHost_->GetSecondaryViewport()->GetSelectedPointIndex()
            : -1;
        int selected = (primaryIdx >= 0) ? primaryIdx : secondaryIdx;
        if (selected < 0) return;
        deletePointAt_(selected);
    };

    // Stage 1 gizmo drag: Viewport fires OnTransformApplied on LMB release with
    // (before, after) target-mesh transforms. Pushing a TransformUndoAction here
    // makes Ctrl+Z reverse the drag in one step, matching v1.0 behaviour.
    auto onTransformApplied = [this](Transform before, Transform after) {
        if (undoStack_ && project_) {
            undoStack_->Push(std::make_unique<TransformUndoAction>(
                project_.get(), before, after));
        }
    };

    // Stage 3 mask paint stroke: Viewport hands us the pre-stroke vertexMask
    // snapshot via OnMaskStrokeCompleted. Push one undo action per gesture.
    auto onMaskStrokeCompleted = [this](std::vector<bool> before) {
        if (!undoStack_ || !project_) return;
        std::vector<bool> after = project_->GetMorphData().vertexMask;
        if (after == before) return; // No-op stroke (e.g. brush radius 0).
        undoStack_->Push(std::make_unique<MaskStrokeUndoAction>(
            project_.get(), std::move(before), std::move(after)));
    };

    // Stage 4 sculpt stroke: Viewport fires OnSculptStrokeCompleted with the
    // BrushStroke (which carries pre-stroke positions for Undo). We capture
    // the post-stroke positions for Redo by diffing against stroke.Undo on a
    // copy of the deformed-mesh vertex buffer.
    auto onSculptStrokeCompleted = [this](BrushStroke stroke) {
        if (!undoStack_ || !project_ || stroke.IsEmpty()) return;
        Mesh* mesh = nullptr;
        if (project_->GetMorphData().deformedMorphMesh) {
            mesh = project_->GetMorphData().deformedMorphMesh.get();
        }
        if (!mesh) return;
        const auto& vertices = mesh->GetVertices();
        auto undone = vertices;
        stroke.Undo(undone);
        std::map<int, Vector3> afterPositions;
        const size_t n = std::min(vertices.size(), undone.size());
        for (size_t i = 0; i < n; ++i) {
            const Vector3& cur = vertices[i];
            const Vector3& bef = undone[i];
            if (cur.x != bef.x || cur.y != bef.y || cur.z != bef.z) {
                afterPositions[static_cast<int>(i)] = cur;
            }
        }
        undoStack_->Push(std::make_unique<SculptStrokeUndoAction>(
            project_.get(), mesh, stroke, afterPositions));
    };

    if (auto* left = viewportHost_->GetPrimaryViewport()) {
        left->OnPointPlaced            = onPointPlaced;
        left->OnPointDeleteRequested   = onPointDelete;
        left->OnTransformApplied       = onTransformApplied;
        left->OnMaskStrokeCompleted    = onMaskStrokeCompleted;
        left->OnSculptStrokeCompleted  = onSculptStrokeCompleted;
    }
    if (auto* right = viewportHost_->GetSecondaryViewport()) {
        right->OnPointPlaced            = onPointPlaced;
        right->OnPointDeleteRequested   = onPointDelete;
        right->OnTransformApplied       = onTransformApplied;
        right->OnMaskStrokeCompleted    = onMaskStrokeCompleted;
        right->OnSculptStrokeCompleted  = onSculptStrokeCompleted;
    }

    glfwSetWindowUserPointer(window_, this);

    glfwSetWindowFocusCallback(window_, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(window_, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetCharCallback(window_, ImGui_ImplGlfw_CharCallback);

    // Mouse gating: the ImGui WantCaptureMouse gate applies only when no drag
    // is in progress. Once a button is held over a viewport, subsequent
    // move/release events bypass the gate — otherwise a drag that crosses
    // into an ImGui panel permanently strands the viewport in orbit/pan state
    // (no RELEASE reaches Viewport::OnMouseButton to clear isOrbiting_).
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods) {
        ImGui_ImplGlfw_MouseButtonCallback(w, button, action, mods);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        const bool gate = !self->viewportHost_->IsDragging();
        if (action == GLFW_PRESS && gate && ImGui::GetIO().WantCaptureMouse) return;
        double sx, sy; glfwGetCursorPos(w, &sx, &sy);
        self->viewportHost_->ForwardMouseButton(button, action, mods, sx, sy);
    });

    glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double sx, double sy) {
        ImGui_ImplGlfw_CursorPosCallback(w, sx, sy);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        // Cursor position is non-mutating — it only updates hover state and
        // the brush cursor overlay. ViewportHost::HitTest already filters to
        // the viewport rect, so we never accidentally drive viewport state
        // from cursor moves over an ImGui panel. Gating on WantCaptureMouse
        // here would block the brush cursor from appearing on hover until
        // the first click into the viewport, which is what users hit before.
        self->viewportHost_->ForwardMouseMove(sx, sy);
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* w, double xOff, double yOff) {
        ImGui_ImplGlfw_ScrollCallback(w, xOff, yOff);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self || !self->viewportHost_) return;
        if (ImGui::GetIO().WantCaptureMouse) return;
        double sx, sy; glfwGetCursorPos(w, &sx, &sy);
        self->viewportHost_->ForwardScroll(xOff, yOff, sx, sy);
    });

    glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
        ImGui_ImplGlfw_KeyCallback(w, key, scancode, action, mods);
        auto* self = static_cast<AppWindow*>(glfwGetWindowUserPointer(w));
        if (!self) return;
        // Application-global shortcuts (Ctrl-modified) are intercepted before
        // the viewport so they fire regardless of keyboard focus. Text-editing
        // ImGui widgets still receive character input through the char callback
        // above, so Ctrl+Z inside a text field would need its own guard — but
        // no editable text widget currently exists in Sprint 3.6.1.
        if (self->HandleGlobalShortcut(key, action, mods)) return;
        if (!self->viewportHost_) return;
        if (ImGui::GetIO().WantCaptureKeyboard) return;
        self->viewportHost_->ForwardKey(key, scancode, action, mods);
    });

    return true;
}

void AppWindow::Run() {
    if (!window_) return;

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);

        TickOnce(fbWidth, fbHeight);
    }
}

void AppWindow::TickOnce(int fbWidth, int fbHeight) {
    if (!window_) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Sprint 6 Story 6.1 -- LoginScreen is the mandatory gate whenever
    // LicenseManager reports Unauthenticated or Authenticating. It must
    // fully replace every other UI surface: otherwise the foreground-
    // drawlist primitives in Viewport::DrawOverlays (orientation gizmo,
    // point labels, active-pane border) bleed through, and the GL 3D
    // pass keeps writing pixels behind the login card. Short-circuit the
    // entire frame body -- render only the login surface and skip the
    // app shell, viewports, sidebar, and modals.
    auto& licenseMgr = LicenseManager::Instance();
    // Sprint 6 Story 6.2 -- drain the deep-link queue (main.cpp argv or
    // SingleInstance onRaise). Story 6.3 -- drain the exchange-worker
    // result if the POST has finished. TickTimeout fires the 5-minute
    // auth watchdog. Order matters: deep-link can spawn the exchange,
    // exchange can transition to Authenticated/Unauthenticated, so all
    // three run before we read the auth state.
    licenseMgr.DrainPendingDeepLink();
    licenseMgr.DrainPendingExchange();
    licenseMgr.TickTimeout();
    if (auto err = licenseMgr.ConsumeLastError(); err && !err->empty()) {
        errorModal_.Open(*err);
    }
    const auto authState = licenseMgr.GetState();
    const bool gateLogin =
        authState == LicenseManager::AuthState::Unauthenticated ||
        authState == LicenseManager::AuthState::Authenticating;
    if (gateLogin) {
        // Map auth state to the LoginScreen UI sub-state. The screen
        // itself is a pure render fn and reports clicks via Result;
        // AppWindow translates those into LicenseManager transitions.
        const LoginScreen::State loginUi =
            (authState == LicenseManager::AuthState::Authenticating)
                ? LoginScreen::State::Waiting
                : LoginScreen::State::Idle;

        // Render login first so the title bar (Begin'd second below)
        // sits on top of it in ImGui's draw order -- drag, minimize,
        // maximize, close need to work even while the user hasn't
        // signed in. The title bar's breadcrumb collapses to
        // "MetaVisage / Sign in" since no project is loaded yet
        // (RenderTitleBar branches on the same auth-state check).
        const auto loginResult = LoginScreen::Render(loginUi, ImGui::GetTime());

        SyncOSWindowTitle();
        RenderTitleBar();
        if (loginResult.signInRequested || loginResult.retryRequested) {
            // Story 6.2 fills BeginAuthFlow in with the browser launch +
            // state token; 6.1 just transitions to Authenticating so the
            // spinner animates. Retry from Waiting reuses the same path.
            licenseMgr.BeginAuthFlow();
        }
        if (loginResult.cancelRequested) {
            licenseMgr.CancelAuthFlow();
        }

        // Still poll worker thread state in case a morph started
        // pre-login. Doesn't render anything; cleans up the thread
        // when it finishes so closing the login resumes a coherent app.
        PollMorphCompletion();

        ImGui::Render();
        // Solid MvBg0 fill for the OS-window letterbox area outside the
        // login card. The login screen itself is opaque and covers
        // every pixel inside the viewport.
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(MvStyle::MvBg0.x, MvStyle::MvBg0.y, MvStyle::MvBg0.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
        return;
    }

    // App shell (dockspace + menu bar + status bar) goes first so that
    // docked windows — the future sidebar and the viewport — can reserve
    // their dock slots before BuildLayout runs.
    RenderAppShell();

    if (viewportHost_) {
        viewportHost_->BuildLayout(
            fbWidth, fbHeight,
            centralRectPos_.x, centralRectPos_.y,
            centralRectSize_.x, centralRectSize_.y);
        // Story 3.8.5 — Stage 3 viewport toolbar shows a 3-state status
        // pill driven by the morph worker + project state. Mirror it
        // into each viewport so the overlay can read without locking.
        //   Processing → worker is running
        //   Complete   → not running, but morph has been processed
        //   Ready      → no morph done yet (or isProcessed false)
        const bool processing = morphRunning_.load();
        const bool processed  = project_ && project_->GetMorphData().isProcessed;
        const Viewport::MorphStatus status =
            processing ? Viewport::MorphStatus::Processing
          : processed  ? Viewport::MorphStatus::Complete
                       : Viewport::MorphStatus::Ready;
        if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
            vp->SetMorphStatus(status);
        }
        if (Viewport* vp = viewportHost_->GetSecondaryViewport()) {
            vp->SetMorphStatus(status);
        }
    }
    RenderFrame(fbWidth, fbHeight);

    // Poll worker thread state before building the sidebar this frame so
    // the progress bar + enable/disable of the Process Morph button
    // reflect current worker status.
    PollMorphCompletion();

    // Stage sidebar docks into the left side of AppDockSpace. Kept after
    // the dockspace is declared (inside RenderAppShell) and before modal
    // rendering so per-frame writes to Project state propagate this frame.
    {
        // Story 3.8.8 — Stage 2 point list highlights whichever
        // correspondence is selected in the viewport. Read from the
        // primary viewport (selection is mirrored across panes by
        // ViewportHost::SetSelectedPointIndex).
        const int selectedPointIdx =
            (viewportHost_ && viewportHost_->GetPrimaryViewport())
                ? viewportHost_->GetPrimaryViewport()->GetSelectedPointIndex()
                : -1;
        auto actions = stageSidebar_.Render(project_.get(),
                                            morphRunning_.load(),
                                            morphProgress_.load(),
                                            userPresets_,
                                            selectedPointIdx);

        if (actions.selectPointIndex && viewportHost_) {
            viewportHost_->SetSelectedPointIndex(*actions.selectPointIndex);
        }
        if (actions.deletePointIndex && deletePointAt_) {
            deletePointAt_(*actions.deletePointIndex);
        }

        if (actions.nextStageRequested && project_) {
            const int cur = static_cast<int>(project_->GetCurrentStage());
            if (cur < static_cast<int>(WorkflowStage::TouchUp)) {
                project_->SetCurrentStage(static_cast<WorkflowStage>(cur + 1));
                ApplyStageViewportMode();
            }
        }
        if (actions.previousStageRequested && project_) {
            const int cur = static_cast<int>(project_->GetCurrentStage());
            if (cur > 0) {
                project_->SetCurrentStage(static_cast<WorkflowStage>(cur - 1));
                ApplyStageViewportMode();
            }
        }
        if (actions.importMorphMeshRequested)  pendingAction_ = PendingAction::ImportMorphMesh;
        if (actions.importTargetMeshRequested) pendingAction_ = PendingAction::ImportTargetMesh;
        if (actions.exportMeshRequested)       pendingAction_ = PendingAction::ExportMesh;
        if (actions.useDefaultMorphMeshRequested) UseDefaultMorphMesh();
        if (!actions.importTextureSlot.empty()) ImportTextureForSlot(actions.importTextureSlot);
        if (actions.resetTransformRequested && project_) {
            if (undoStack_) {
                undoStack_->Push(std::make_unique<TransformUndoAction>(
                    project_.get(),
                    project_->GetTargetMesh().transform,
                    Transform()));
            }
            project_->GetTargetMesh().transform = Transform();
            project_->GetAlignmentData().targetMeshTransform = Transform();
        }
        if (actions.transformEditCompleted && project_ && undoStack_) {
            undoStack_->Push(std::make_unique<TransformUndoAction>(
                project_.get(),
                actions.transformEditBefore,
                actions.transformEditAfter));
        }
        if (actions.clearAllPointsRequested && project_) {
            project_->GetPointReferenceData().correspondences.clear();
        }
        if (actions.processMorphRequested) StartMorph();
        if (actions.cancelMorphRequested)  CancelMorph();
        if (actions.acceptMorphRequested)  AcceptMorph();
        // finalize is a Sprint 4 no-op (Export handles end-of-workflow).

        // Story 4.4 — Wrap preset card row actions.
        if (actions.savePresetRequested) {
            std::vector<std::string> takenNames;
            takenNames.reserve(userPresets_.size());
            for (const auto& p : userPresets_) takenNames.push_back(p.name);
            savePresetModal_.Open("Save Preset", /*editingId=*/"",
                                  /*name=*/"", /*desc=*/"", /*glyph=*/"",
                                  std::move(takenNames));
        }
        if (actions.renamePresetRequested && !actions.targetPresetId.empty()) {
            const WrapPreset* target = nullptr;
            for (const auto& p : userPresets_) {
                if (p.id == actions.targetPresetId) { target = &p; break; }
            }
            if (target) {
                std::vector<std::string> takenNames;
                for (const auto& p : userPresets_) {
                    // Exclude the preset being renamed from the collision list
                    // — keeping its current name is a no-op rename.
                    if (p.id != target->id) takenNames.push_back(p.name);
                }
                savePresetModal_.Open("Rename Preset", target->id,
                                      target->name, target->description,
                                      target->iconGlyph, std::move(takenNames));
            }
        }
        if (actions.deletePresetRequested && !actions.targetPresetId.empty()) {
            const WrapPreset* target = nullptr;
            for (const auto& p : userPresets_) {
                if (p.id == actions.targetPresetId) { target = &p; break; }
            }
            if (target) {
                const std::string presetId = target->id;
                confirmModal_.Open(
                    "Delete preset?",
                    std::format("Delete preset '{}'? This cannot be undone.",
                                target->name),
                    [this, presetId]() {
                        PresetStore::Delete(presetId);
                        userPresets_ = PresetStore::LoadAll();
                    },
                    "Delete");
            }
        }
        if (actions.replacePresetRequested && !actions.targetPresetId.empty() && project_) {
            for (const auto& p : userPresets_) {
                if (p.id != actions.targetPresetId) continue;
                // Build the replacement preset on the side; only mutate
                // userPresets_ after Save succeeds so a write failure
                // (full disk, permission denied) doesn't leave the
                // in-memory list out of sync with what's on disk.
                WrapPreset updated = p;
                updated.params = WrapParams::FromMorphData(project_->GetMorphData());
                if (PresetStore::Save(updated)) {
                    userPresets_ = PresetStore::LoadAll();
                    statusMessage_ = std::format("Replaced preset '{}'", updated.name);
                } else {
                    errorModal_.Open(std::format(
                        "Could not save preset '{}' to disk. "
                        "Check %APPDATA%\\MetaVisage\\presets is writable.",
                        updated.name));
                }
                break;
            }
        }

        // Mirror Stage 3 mask-UI state onto the primary viewport so LMB,
        // renderer overlay, and brush radius track sidebar toggles each
        // frame without a separate signal bus.
        if (viewportHost_ && project_) {
            if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
                const bool inMorph = project_->GetCurrentStage() == WorkflowStage::Morph;
                vp->SetMaskPaintingMode(inMorph && actions.maskPaintingActive);
                vp->SetShowMask(inMorph && actions.maskShow);
                vp->SetMaskBrushRadius(actions.maskBrushRadius);
                vp->SetMaskEraseMode(actions.maskEraseMode);
                if (inMorph) {
                    vp->SetMaskData(&project_->GetMorphData().vertexMask);
                    // Push the preview-mode combo selection through to the
                    // renderer. The sidebar writes MorphData::previewMode
                    // directly; without this mirror call the dropdown
                    // updates state but never rebinds the renderer.
                    vp->SetMorphPreviewMode(project_->GetMorphData().previewMode);
                }
            }
        }

        // Mirror Stage 4 sculpt state onto the primary viewport every
        // frame so brush type/radius/strength/falloff/symmetry stay in
        // sync with the sidebar sliders.
        if (viewportHost_ && project_ &&
            project_->GetCurrentStage() == WorkflowStage::TouchUp) {
            if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
                const auto& s = project_->GetSculptState();
                vp->SetBrushType(s.activeBrush);
                vp->SetBrushRadius(s.brushRadius);
                vp->SetBrushStrength(s.brushStrength);
                vp->SetBrushFalloff(s.falloff);
                vp->SetSculptSymmetry(s.symmetryEnabled, s.symmetryAxis);
                vp->SetShowTargetOverlay(s.showTargetOverlay);
            }
        }

        // Mask edit commands — operate directly on MorphData; upload new
        // colours to the renderer so the overlay updates in-place.
        if ((actions.clearMaskRequested || actions.invertMaskRequested) && project_) {
            MorphData& md = project_->GetMorphData();
            if (actions.clearMaskRequested) {
                std::fill(md.vertexMask.begin(), md.vertexMask.end(), false);
            }
            if (actions.invertMaskRequested) {
                for (size_t i = 0; i < md.vertexMask.size(); ++i) {
                    md.vertexMask[i] = !md.vertexMask[i];
                }
            }
            if (viewportHost_) {
                if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
                    // Pick whichever mesh the mask is currently being
                    // rendered on (mirrors Viewport::GetMaskMesh).
                    Mesh* m = md.isProcessed && md.deformedMorphMesh
                                  ? md.deformedMorphMesh.get()
                                  : project_->GetMorphMesh().mesh.get();
                    if (m) vp->UploadMaskColors(m, md.vertexMask);
                }
            }
        }
    }

    // First frame after init: surface the Welcome modal if we have no
    // project content yet. Skipped if Ctrl+N / an opened project has
    // already populated state.
    if (!welcomeShownOnLaunch_) {
        welcomeShownOnLaunch_ = true;
        if (project_ && project_->GetMorphMesh().filepath.empty() &&
            project_->GetPath().empty()) {
            welcomeModal_.Open();
        }
    }

    DispatchPendingAction();
    RenderModals();

    // Story 3.8.12 — login screen takes over the entire frame at the
    // top of TickOnce (see early-return branch above). When that branch
    // fires we never reach this point.

    ImGui::Render();

    glViewport(0, 0, fbWidth, fbHeight);
    // OS framebuffer clear matches MvBg0 (deep navy). Anywhere ImGui
    // doesn't paint and the viewport's GL pass doesn't cover (1-2 px
    // gaps between the host window's NoBackground area and adjacent
    // docked windows) used to leak the legacy 30/255 ImGui dark-grey
    // through, which read as "#1e1e1e gray bars" between the stepper /
    // dockspace / status bar. Clearing to MvBg0 makes those gaps
    // invisible against the design palette.
    glClearColor(MvStyle::MvBg0.x, MvStyle::MvBg0.y, MvStyle::MvBg0.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (viewportHost_) viewportHost_->RenderGL(fbHeight);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
}

void AppWindow::RenderAppShell() {
    // Story 3.8.2 — chrome layout:
    //   ┌─────────────────────────────────────────┐  ← OS native title bar
    //   │ ◇ MetaVisage · file.mmproj > STAGE 01 … │ 36 px ##MvTitleBar window
    //   ├─────────────────────────────────────────┤
    //   │ File Edit View Mesh Help …  Saved · LL │ ~26 px BeginMenuBar
    //   ├─────────────────────────────────────────┤
    //   │  DockSpace (sidebar + viewport)         │
    //   ├─────────────────────────────────────────┤
    //   │ ● Ready · Stage 01 ALIGN · …            │ 28 px status bar
    //   └─────────────────────────────────────────┘
    //
    // The title bar lives in its own borderless window pinned to the top of
    // the main viewport; the host window starts 36 px below it. Keeping the
    // title bar separate avoids fighting ImGui's BeginMenuBar layout (which
    // owns the top of any window with WindowFlags_MenuBar).
    constexpr float TitleBarH = 36.0f;
    SyncOSWindowTitle();
    RenderTitleBar();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + TitleBarH));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, vp->Size.y - TitleBarH));
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        // NoScrollbar / NoScrollWithMouse — the host window must never
        // scroll. ImGui's ItemSpacing.y between the menu bar / stepper
        // child / dockspace / status bar can push total content a few
        // pixels over the host's size, which triggered a vertical
        // scrollbar on the entire app and let users drag chrome out of
        // alignment with the design hairlines. Inner BeginChild regions
        // (stage body, point list, etc.) still scroll on their own.
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        // NoBackground is essential — combined with PassthruCentralNode on the
        // dockspace below, the central (viewport) area becomes transparent so
        // the 3D GL pass shows through. Without this the host's grey fill hid
        // the grid and imported meshes entirely.
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##AppHostWindow", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    // Push ItemSpacing.y = 0 for the host's direct children (menu bar,
    // stepper child, dockspace, status bar). The default ItemSpacing.y
    // of 6 px inserted hairline gaps between those layout blocks where
    // the host's NoBackground transparency leaked the framebuffer
    // colour through, reading as "spacer bars" between the chrome
    // sections. Popping before End() keeps the per-stage sidebar's own
    // Begin scope on the un-pushed default spacing.
    const ImVec2 prevSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(prevSpacing.x, 0.0f));

    RenderMainMenuBar();

    // Story 3.8.3 — full-width stage stepper between the menu bar and the
    // dockspace. Returns the 0-based stage index the user clicked, or
    // nullopt. Forward-jump validation lives inside StageStepper::Render
    // (mirroring StageSidebar's `Next >` gate); we re-validate the result
    // here as a belt-and-braces guard before mutating Project state.
    if (auto clicked = StageStepper::Render(project_.get(),
                                            ImGui::GetContentRegionAvail().x)) {
        if (project_) {
            const int target  = *clicked;
            const int current = static_cast<int>(project_->GetCurrentStage());
            const bool allowed =
                (target != current) &&
                (target < current ||
                 (target == current + 1 && project_->CanProceedToNextStage()));
            if (allowed) {
                project_->SetCurrentStage(static_cast<WorkflowStage>(target));
                ApplyStageViewportMode();
            }
        }
    }

    // Reserve a 28 px row for the status bar at the bottom of the host window.
    // Spec value (Sprint 3.8 design); the previous GetFrameHeight() reservation
    // was font-size-dependent, which would flicker the dockspace size on font
    // changes between frames during font-atlas reloads.
    constexpr float statusH = 28.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float bodyH = avail.y - statusH;

    // Story 3.8.4 — left tool rail for Stage 1 (transforms) and Stage 4
    // (sculpt brushes). Rendered as a 56 px-wide BeginChild to the left of
    // the dockspace; SameLine then docks the dockspace flush against it.
    // Stages 2 (Point Reference) and 3 (Morph) hide the rail entirely so
    // the dockspace claims the full body width.
    const WorkflowStage curStage =
        project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;
    const bool railVisible = (curStage == WorkflowStage::Alignment ||
                              curStage == WorkflowStage::TouchUp);
    float railW = 0.0f;
    if (railVisible) {
        Viewport* primaryViewport =
            viewportHost_ ? viewportHost_->GetPrimaryViewport() : nullptr;
        ToolRail::Result railResult =
            ToolRail::Render(curStage, project_.get(), primaryViewport, bodyH);
        if (railResult.helpRequested) {
            pendingAction_ = PendingAction::ShowKeyboardShortcuts;
        }
        railW = ToolRail::WidthPx;
        ImGui::SameLine(0.0f, 0.0f);
    }

    ImVec2 dockSize(avail.x - railW, bodyH);

    ImGuiID dockId = ImGui::GetID("AppDockSpace");
    ImGui::DockSpace(dockId, dockSize, ImGuiDockNodeFlags_PassthruCentralNode);

    // First-run dock layout: split the dockspace, pin "Stage" to the right,
    // and lock the right node (no undock / no resize / no foreign docking).
    // Rebuilt every launch so an accidental imgui.ini edit can't free the
    // sidebar — the sidebar is meant to be static UI, not a dockable panel.
    static bool sDockBuilt = false;
    if (!sDockBuilt) {
        sDockBuilt = true;
        ImGui::DockBuilderRemoveNode(dockId);
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockId, dockSize);

        ImGuiID rightId = 0;
        ImGuiID centerId = 0;
        ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Right, 0.28f, &rightId, &centerId);

        if (ImGuiDockNode* rightNode = ImGui::DockBuilderGetNode(rightId)) {
            rightNode->LocalFlags |=
                ImGuiDockNodeFlags_NoUndocking |
                ImGuiDockNodeFlags_NoDockingOverMe |
                ImGuiDockNodeFlags_NoDockingSplit |
                ImGuiDockNodeFlags_HiddenTabBar;
        }
        if (ImGuiDockNode* centerNode = ImGui::DockBuilderGetNode(centerId)) {
            centerNode->LocalFlags |=
                ImGuiDockNodeFlags_NoDockingOverMe |
                ImGuiDockNodeFlags_NoDockingSplit |
                ImGuiDockNodeFlags_NoTabBar;
        }

        ImGui::DockBuilderDockWindow("Stage", rightId);
        ImGui::DockBuilderFinish(dockId);
    }

    // Capture the central-node rect so the viewport's GL pass scissors to the
    // visible central region (not the full work area). Without this, the GL
    // pass draws under the sidebar and the orientation gizmo in the top-right
    // corner of the viewport gets covered by the sidebar.
    if (ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockId)) {
        centralRectPos_ = centralNode->Pos;
        centralRectSize_ = centralNode->Size;
    } else {
        centralRectPos_ = vp->WorkPos;
        centralRectSize_ = vp->WorkSize;
    }

    // Status bar — rendered as the final child of the host window so it sits
    // flush at the bottom. No separate top-level window needed.
    ImGui::SetCursorPosY(ImGui::GetCursorPosY());
    RenderStatusBar();

    ImGui::PopStyleVar(); // ItemSpacing pushed at top of host body
    ImGui::End();
}

// =============================================================================
// Story 3.8.2 — chrome rendering
// =============================================================================

namespace {

// Compose breadcrumb text used in both the title bar and the OS taskbar.
// Returns "Stage N / 4 · STAGENAME" — caller decides upper/lower casing.
std::string StageBreadcrumb(WorkflowStage s) {
    return std::format("Stage {} / 4 · {}",
                       static_cast<int>(s) + 1,
                       StageDisplayName(s));
}

std::string ProjectFileLabel(const std::string& projectPath) {
    if (projectPath.empty()) return "Untitled Project";
    return std::filesystem::path(projectPath).filename().string();
}

// "Saved 12s ago" / "Saved 3m ago" / "Saved <1h ago" / "Unsaved".
std::string SavedAgoLabel(const std::optional<std::chrono::steady_clock::time_point>& at) {
    if (!at) return "Unsaved";
    using namespace std::chrono;
    const auto delta = duration_cast<seconds>(steady_clock::now() - *at).count();
    if (delta < 5)    return "Saved just now";
    if (delta < 60)   return std::format("Saved {}s ago",  delta);
    if (delta < 3600) return std::format("Saved {}m ago",  delta / 60);
    return std::format("Saved {}h ago", delta / 3600);
}

// Convert an ImVec4 to ImGui packed RGBA32 — used heavily by the title bar
// drawlist commands.
ImU32 Col(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

// Caption-button hit area on the right of the title bar. Renders the icon
// glyph (min/max/close) via ImDrawList primitives and dispatches the GLFW
// action on click. Hover background is MvBg2 except for close which uses
// the Windows red.
void CaptionButton(GLFWwindow* win, ImDrawList* dl, ImVec2 pmin, ImVec2 pmax,
                   int kind /*0=min, 1=max, 2=close*/) {
    ImGui::SetCursorScreenPos(pmin);
    ImGui::PushID(kind);
    const bool clicked = ImGui::InvisibleButton(
        "##cap", ImVec2(pmax.x - pmin.x, pmax.y - pmin.y));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    if (hovered) {
        const ImU32 bg = (kind == 2)
            ? IM_COL32(0xE8, 0x11, 0x23, 0xFF)            // Windows close-red
            : Col(MetaVisage::MvStyle::MvBg2);
        dl->AddRectFilled(pmin, pmax, bg);
    }

    const ImU32 iconCol = (kind == 2 && hovered)
        ? IM_COL32_WHITE
        : Col(MetaVisage::MvStyle::MvFg2);
    const float cx = (pmin.x + pmax.x) * 0.5f;
    const float cy = (pmin.y + pmax.y) * 0.5f;
    const float halfIcon = 5.0f;

    if (kind == 0) {
        dl->AddLine(ImVec2(cx - halfIcon, cy + halfIcon * 0.5f),
                    ImVec2(cx + halfIcon, cy + halfIcon * 0.5f),
                    iconCol, 1.6f);
    } else if (kind == 1) {
        const bool isMaxed = win && glfwGetWindowAttrib(win, GLFW_MAXIMIZED);
        if (isMaxed) {
            // Two stacked rects suggesting "restore"
            dl->AddRect(ImVec2(cx - halfIcon + 1, cy - halfIcon),
                        ImVec2(cx + halfIcon,     cy + halfIcon - 1),
                        iconCol, 0.0f, 0, 1.4f);
            dl->AddRect(ImVec2(cx - halfIcon,     cy - halfIcon + 1),
                        ImVec2(cx + halfIcon - 1, cy + halfIcon),
                        iconCol, 0.0f, 0, 1.4f);
        } else {
            dl->AddRect(ImVec2(cx - halfIcon, cy - halfIcon),
                        ImVec2(cx + halfIcon, cy + halfIcon),
                        iconCol, 0.0f, 0, 1.4f);
        }
    } else { // close
        const float h = halfIcon - 0.5f;
        dl->AddLine(ImVec2(cx - h, cy - h), ImVec2(cx + h, cy + h), iconCol, 1.6f);
        dl->AddLine(ImVec2(cx + h, cy - h), ImVec2(cx - h, cy + h), iconCol, 1.6f);
    }

    if (clicked && win) {
        if (kind == 0) {
            glfwIconifyWindow(win);
        } else if (kind == 1) {
            if (glfwGetWindowAttrib(win, GLFW_MAXIMIZED)) glfwRestoreWindow(win);
            else                                          glfwMaximizeWindow(win);
        } else {
            glfwSetWindowShouldClose(win, GLFW_TRUE);
        }
    }
}

} // namespace

void AppWindow::SyncOSWindowTitle() {
    if (!window_) return;
    const WorkflowStage stage = project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;
    std::string title = std::format("MetaVisage — {} — Stage {} / 4 · {}",
                                    ProjectFileLabel(projectPath_),
                                    static_cast<int>(stage) + 1,
                                    StageDisplayName(stage));
    if (title != lastOSTitle_) {
        glfwSetWindowTitle(window_, title.c_str());
        lastOSTitle_ = std::move(title);
    }
}

void AppWindow::RenderTitleBar() {
    constexpr float TitleBarH = 36.0f;
    constexpr float CaptionW  = 46.0f;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, TitleBarH));
    ImGui::SetNextWindowViewport(vp->ID);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, MvStyle::MvBg0);

    ImGui::Begin("##MvTitleBar", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 wp = ImGui::GetWindowPos();
    const ImVec2 ws = ImGui::GetWindowSize();

    // 1 px hairline divider between the title bar and whatever sits below
    // (currently the menu bar in the host window).
    dl->AddLine(ImVec2(wp.x,         wp.y + ws.y - 1.0f),
                ImVec2(wp.x + ws.x,  wp.y + ws.y - 1.0f),
                Col(MvStyle::MvLine2));

    // ---- Caption buttons (right edge) -----------------------------------
    // Drawn first so we know how much horizontal room remains for the
    // breadcrumb. Buttons stack right-to-left: close, max, min.
    const float captionRight = ws.x;
    for (int i = 0; i < 3; ++i) {
        const float x1 = captionRight - i * CaptionW;
        const float x0 = x1 - CaptionW;
        // i=0 → close (rightmost), i=1 → max, i=2 → min
        const int kind = (i == 0) ? 2 : (i == 1) ? 1 : 0;
        CaptionButton(window_, dl,
                      ImVec2(wp.x + x0, wp.y),
                      ImVec2(wp.x + x1, wp.y + ws.y),
                      kind);
    }
    const float breadcrumbMaxX = ws.x - 3.0f * CaptionW - 8.0f;

    // ---- Logo -----------------------------------------------------------
    constexpr float LeftPad = 12.0f;
    constexpr float LogoSize = 18.0f;
    const float logoY = (TitleBarH - LogoSize) * 0.5f;
    if (MvStyle::Asset.LogoTextureId) {
        ImGui::SetCursorPos(ImVec2(LeftPad, logoY));
        // Standard ImGui-OpenGL-backend pattern: pass the GL texture name
        // through ImTextureID via an intptr_t round-trip. Modern ImGui
        // defines ImTextureID as ImU64; reinterpret_cast across the int↔ptr
        // boundary is rejected by MSVC, so a C-style cast is canonical here.
        ImGui::Image((ImTextureID)(intptr_t)MvStyle::Asset.LogoTextureId,
                     ImVec2(LogoSize, LogoSize));
    }

    // ---- Breadcrumb text ------------------------------------------------
    // Vertically center against the body font's height; if the design fonts
    // aren't loaded the fallback is ProggyClean which is shorter, so we use
    // the live font size rather than a hardcoded 14 px.
    const float textH = ImGui::GetFontSize();
    const float textY = (TitleBarH - textH) * 0.5f;
    float cx = LeftPad + LogoSize + 10.0f;

    auto drawSegment = [&](const std::string& s, ImFont* font, const ImVec4& color) {
        if (cx > breadcrumbMaxX) return;
        ImGui::SetCursorPos(ImVec2(cx, textY));
        MvStyle::ScopedFont f(font);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        // Manually clip if the segment would overflow into the caption-button
        // region. Easier than dynamic ellipsing for a chrome row that rarely
        // overflows on real-world filenames.
        const float available = breadcrumbMaxX - cx;
        const float width = ImGui::CalcTextSize(s.c_str()).x;
        if (width > available) {
            ImGui::PushClipRect(ImVec2(ImGui::GetCursorScreenPos().x,
                                       ImGui::GetCursorScreenPos().y - 4),
                                ImVec2(ImGui::GetCursorScreenPos().x + available,
                                       ImGui::GetCursorScreenPos().y + textH + 4),
                                true);
            ImGui::TextUnformatted(s.c_str());
            ImGui::PopClipRect();
            cx = breadcrumbMaxX + 1.0f;
        } else {
            ImGui::TextUnformatted(s.c_str());
            cx += width;
        }
        ImGui::PopStyleColor();
    };

    drawSegment("MetaVisage", MvStyle::Font.BodyMd, MvStyle::MvFg1);
    cx += 10.0f;
    drawSegment("\xC2\xB7", MvStyle::Font.Body, MvStyle::MvLine3);   // U+00B7 middot
    cx += 10.0f;

    // Sprint 6 Story 6.1 -- during login the breadcrumb is just
    // "MetaVisage / Sign in". No project is loaded yet and there's no
    // active stage to surface, so we mirror the design's login.jsx
    // title bar shorthand. The drag/close/min/max plumbing in
    // RenderTitleBar (caption buttons + WM_NCHITTEST hook) keeps
    // working unchanged.
    {
        const auto authState = LicenseManager::Instance().GetState();
        if (authState == LicenseManager::AuthState::Unauthenticated ||
            authState == LicenseManager::AuthState::Authenticating) {
            drawSegment("Sign in", MvStyle::Font.Body, MvStyle::MvFg2);
            // Caption buttons drawn below; skip the rest of the breadcrumb.
            ImGui::End();
            return;
        }
    }

    const WorkflowStage stage = project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;

    drawSegment(ProjectFileLabel(projectPath_), MvStyle::Font.Body, MvStyle::MvFg2);
    cx += 10.0f;
    drawSegment(">", MvStyle::Font.Body, MvStyle::MvLine3);
    cx += 10.0f;

    // STAGE NN / 4 · STAGENAME — uppercased mono in MvBlue400
    std::string stageLabel = std::format("STAGE {:02d} / 4 \xC2\xB7 ",
                                         static_cast<int>(stage) + 1);
    const std::string stageName = StageDisplayName(stage);
    for (char c : stageName) stageLabel.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    drawSegment(stageLabel, MvStyle::Font.Mono, MvStyle::MvBlue400);

    ImGui::End();
}

void AppWindow::RenderMainMenuBar() {
    if (!ImGui::BeginMenuBar()) return;

    const bool canUndo = undoStack_ && undoStack_->CanUndo();
    const bool canRedo = undoStack_ && undoStack_->CanRedo();

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project", "Ctrl+N")) OnFileNewProject();
        if (ImGui::MenuItem("Open Project...", "Ctrl+O")) pendingAction_ = PendingAction::OpenProject;
        ImGui::Separator();
        if (ImGui::MenuItem("Save Project", "Ctrl+S")) pendingAction_ = PendingAction::SaveProject;
        if (ImGui::MenuItem("Save Project As...", "Ctrl+Shift+S")) pendingAction_ = PendingAction::SaveProjectAs;
        ImGui::Separator();
        // Import Morph Mesh / Import Target Mesh / Export Mesh remain
        // stage-gated in the sidebar — duplicating them here confused users
        // about when they apply. Decision kept in Sprint 3.6.
        if (ImGui::MenuItem("Exit", "Alt+F4")) OnFileExit();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        ImGui::BeginDisabled(!canUndo);
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) OnEditUndo();
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!canRedo);
        if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) OnEditRedo();
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        // Shading mode lives on the viewport overlay (Stage-3 Story 3.8.5
        // moves it into the new top-centre toolbar). The grid and camera
        // presets remain stubbed for a future story.
        ImGui::MenuItem("Show Grid", nullptr, false, false);
        ImGui::Separator();
        ImGui::MenuItem("Front View (Numpad 1)", nullptr, false, false);
        ImGui::MenuItem("Right View (Numpad 3)", nullptr, false, false);
        ImGui::MenuItem("Top View (Numpad 7)", nullptr, false, false);
        ImGui::MenuItem("Perspective (Home)", nullptr, false, false);
        ImGui::EndMenu();
    }

    // Story 3.8.2 — new "Mesh" menu, scaffolded with the entries the design
    // file shows. All items are stubbed disabled until the corresponding
    // backend lands; surfacing them now matches the design and lets keyboard
    // users discover the planned commands.
    if (ImGui::BeginMenu("Mesh")) {
        ImGui::MenuItem("Recompute Normals",   nullptr, false, false);
        ImGui::MenuItem("Validate Topology",   nullptr, false, false);
        ImGui::Separator();
        ImGui::MenuItem("Center on Origin",    nullptr, false, false);
        ImGui::MenuItem("Snap to Floor",       nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Documentation")) pendingAction_ = PendingAction::OpenDocumentation;
        if (ImGui::MenuItem("Account & License")) pendingAction_ = PendingAction::OpenAccountLicense;
        if (ImGui::MenuItem("Check for Updates...")) pendingAction_ = PendingAction::CheckForUpdates;
        ImGui::Separator();
        if (ImGui::MenuItem("Keyboard Shortcuts")) pendingAction_ = PendingAction::ShowKeyboardShortcuts;
        if (ImGui::MenuItem("About MetaVisage")) pendingAction_ = PendingAction::ShowAbout;
        ImGui::Separator();
        // Sprint 3.8.5 dev aid — browse the Lucide icon font's loaded glyphs
        // and copy codepoints into MvIcons.h. Will move behind a debug build
        // flag once the icon constants are stable.
        ImGui::MenuItem("Icon Glyph Explorer (Dev)", nullptr, &showIconExplorer_);
        // Sprint 6.1 retired the dev-only "Login Screen" toggle that
        // lived here pre-auth-backend. The login surface is now driven
        // by LicenseManager::GetState() == Unauthenticated.
        ImGui::EndMenu();
    }

    // ---- Right-side cluster ---------------------------------------------
    // Save indicator (mono, MvFg3). The Story 3.8.2 "UE 5.4 · LIVE LINK"
    // pill that previously sat to the right of the save label has been
    // dropped — Live Link is far-future scope and the green pill confused
    // users into thinking the connection was real. The previous stage-
    // indicator from Sprint 3.6 already moved into the title bar.
    const std::string savedLabel = SavedAgoLabel(lastSavedAt_);

    float savedW = 0.0f;
    {
        MvStyle::ScopedFont f(MvStyle::Font.Mono);
        savedW = ImGui::CalcTextSize(savedLabel.c_str()).x;
    }
    const float wantW = savedW + 10.0f;
    const float available = ImGui::GetContentRegionAvail().x;
    if (available < wantW) {
        ImGui::EndMenuBar();
        return;
    }

    ImGui::SameLine(ImGui::GetCursorPosX() + (available - wantW));
    {
        MvStyle::ScopedFont f(MvStyle::Font.Mono);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
        ImGui::TextUnformatted(savedLabel.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::EndMenuBar();
}

void AppWindow::RenderStatusBar() {
    constexpr float StatusH = 28.0f;
    const ImVec2 startCursor = ImGui::GetCursorPos();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // Background fill: MvBg1 — same as the sidebar's WindowBg so the
    // footer reads as one continuous surface with the sidebar's body
    // when the eye scans down the right side. Earlier MvBg0 produced
    // a visible colour transition between the sidebar (MvBg1) and the
    // status bar (MvBg0). Drawn via the host window's drawlist rather
    // than a child window so the status bar inherits the host's clipping
    // and doesn't introduce a separate scroll region.
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float fullW = ImGui::GetContentRegionAvail().x;
    dl->AddRectFilled(origin,
                      ImVec2(origin.x + fullW, origin.y + StatusH),
                      Col(MvStyle::MvBg1));

    const WorkflowStage stage = project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;

    // Pad in horizontally and centre vertically.
    constexpr float SidePad = 14.0f;
    MvStyle::ScopedFont monoFont(MvStyle::Font.Mono);
    const float textH = ImGui::GetFontSize();
    const float textY = startCursor.y + (StatusH - textH) * 0.5f;

    // Helper: draw "label value" with custom colors and advance cursor.
    float cx = startCursor.x + SidePad;
    auto drawSection = [&](const std::string& text, const ImVec4& color) {
        ImGui::SetCursorPos(ImVec2(cx, textY));
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopStyleColor();
        cx += ImGui::CalcTextSize(text.c_str()).x;
    };
    auto drawSep = [&]() {
        ImGui::SetCursorPos(ImVec2(cx + 8.0f, textY));
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvLine3);
        ImGui::TextUnformatted("\xC2\xB7"); // U+00B7 middot
        ImGui::PopStyleColor();
        cx += 8.0f + ImGui::CalcTextSize("\xC2\xB7").x + 8.0f;
    };

    // ● Ready (or current statusMessage_)
    drawSection("\xE2\x97\x8F ", MvStyle::MvFg1);
    drawSection(statusMessage_.empty() ? "Ready" : statusMessage_, MvStyle::MvFg2);
    drawSep();

    // Stage 0N STAGENAME
    drawSection("Stage ", MvStyle::MvFg3);
    {
        std::string s = std::format("{:02d} ", static_cast<int>(stage) + 1);
        for (char c : std::string(StageDisplayName(stage)))
            s.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        drawSection(s, MvStyle::MvBlue400);
    }
    drawSep();

    // Source filename (or em dash placeholder)
    drawSection("Source ", MvStyle::MvFg3);
    {
        const std::string src = (project_ && !project_->GetMorphMesh().filepath.empty())
            ? std::filesystem::path(project_->GetMorphMesh().filepath).filename().string()
            : "\xE2\x80\x94"; // em dash
        drawSection(src, MvStyle::MvFg1);
    }
    drawSep();

    // Target filename (or em dash placeholder)
    drawSection("Target ", MvStyle::MvFg3);
    {
        const std::string tgt = (project_ && !project_->GetTargetMesh().filepath.empty())
            ? std::filesystem::path(project_->GetTargetMesh().filepath).filename().string()
            : "\xE2\x80\x94";
        drawSection(tgt, MvStyle::MvFg1);
    }

    // ---- Right-aligned cluster: GPU readout ------------------------------
    // No FPS counter — Sprint 1.4 decision retained per Sprint 3.8 plan.
    // The Story 3.8.2 "Live link UE 5.4 ●" segment that previously followed
    // the GPU label has been dropped — Live Link is far-future scope and
    // the green dot read as a real connection status.
    const std::string rightGpuLabel = std::format("GPU {}", gpuRenderer_);
    const float gpuW = ImGui::CalcTextSize(rightGpuLabel.c_str()).x;
    const float rightCluster = gpuW + SidePad;
    const float rightStartX  = startCursor.x + fullW - rightCluster;
    if (rightStartX > cx + 16.0f) {
        cx = rightStartX;
        drawSection(rightGpuLabel, MvStyle::MvFg2);
    }

    // Reserve the bar's height so the host's content cursor advances past
    // the manually-positioned text (otherwise End() leaves stale layout).
    ImGui::SetCursorPos(startCursor);
    ImGui::Dummy(ImVec2(fullW, StatusH));
}

void AppWindow::DispatchPendingAction() {
    // Actions consumed synchronously here — the menu bar sets pendingAction_
    // during BeginMenu() / BeginMenuBar() traversal, then we run the blocking
    // NFD or kick off the corresponding modal on the same frame. Unblocking
    // modals (Export, Shortcuts) handle their own open-via-pendingOpen path.
    const PendingAction action = pendingAction_;
    if (action == PendingAction::None) return;
    pendingAction_ = PendingAction::None;

    switch (action) {
        case PendingAction::None: break;

        case PendingAction::OpenProject: {
            if (auto picked = FileDialogs::OpenFile(ProjectFilters())) {
                OpenProjectFromDisk(*picked);
            }
            break;
        }
        case PendingAction::SaveProject: {
            // Ctrl+S: silent save to the current path. If no path is known
            // yet, fall through to Save-As behaviour.
            if (!projectPath_.empty()) {
                SaveProjectToDisk(projectPath_);
                break;
            }
            // fall through to SaveProjectAs prompt below
            [[fallthrough]];
        }
        case PendingAction::SaveProjectAs: {
            // Ctrl+Shift+S or "Save As...": always prompt for a new path so
            // the user can save a copy under a different name.
            const std::string suggested = !projectPath_.empty()
                ? FileName(projectPath_)
                : (project_ && !project_->GetName().empty()
                    ? project_->GetName() + ".mmproj"
                    : std::string("untitled.mmproj"));
            if (auto picked = FileDialogs::SaveFile(ProjectFilters(), suggested)) {
                std::string target = *picked;
                if (!EndsWithICase(target, ".mmproj")) target += ".mmproj";
                SaveProjectToDisk(target);
            }
            break;
        }
        case PendingAction::ImportMorphMesh: {
            if (auto picked = FileDialogs::OpenFile(MeshImportFilters())) {
                ImportMorphMeshFromDisk(*picked);
            }
            break;
        }
        case PendingAction::ImportTargetMesh: {
            if (auto picked = FileDialogs::OpenFile(MeshImportFilters())) {
                ImportTargetMeshFromDisk(*picked);
            }
            break;
        }
        case PendingAction::ExportMesh: {
            const bool hasTextures = project_ && project_->GetTargetTextures().HasAlbedo();
            exportModal_.Open(hasTextures);
            break;
        }
        case PendingAction::ShowKeyboardShortcuts:
            shortcutsModal_.Open();
            break;
        case PendingAction::ShowAbout:
            showAbout_ = true;
            break;
        case PendingAction::OpenDocumentation:
            if (!OpenUrl("https://metavisage.io/docs")) {
                errorModal_.Open("Could not open the documentation link in your browser.");
            }
            break;
        case PendingAction::OpenAccountLicense:
            if (!OpenUrl("https://metavisage.io/account")) {
                errorModal_.Open("Could not open the account page in your browser.");
            }
            break;
        case PendingAction::CheckForUpdates:
            // WinSparkle integration is scheduled for the Sprint 3.6 close-out;
            // for now, direct the user to the releases page.
            if (!OpenUrl("https://metavisage.io/releases")) {
                errorModal_.Open("Could not open the releases page.");
            }
            break;
    }
}

// Sprint 3.8.5 dev tool — Lucide icon glyph explorer. Lists every PUA glyph
// the Lucide font actually rasterized, lets the user click any glyph to copy
// its codepoint to the clipboard. Used to populate MvIcons.h once the
// project's Lucide build is identified. Toggle via Help > Icon Glyph
// Explorer (Dev).
//
// Design notes:
// - We walk U+E000..U+F8FF and use ImFont::FindGlyphNoFallback so we don't
//   accidentally enumerate every codepoint in the range (which would then
//   fall back to the missing-glyph rect).
// - Glyph buttons are 40 px square in the icon font; codepoint label below
//   uses MonoSm so the hex is readable without dwarfing the glyph.
// - A text filter narrows by hex prefix (e.g. "EA" jumps to that block).
void AppWindow::RenderIconExplorer() {
    if (!showIconExplorer_) return;

    if (!ImGui::Begin("Lucide Icon Glyph Explorer", &showIconExplorer_,
                      ImGuiWindowFlags_NoDocking)) {
        ImGui::End();
        return;
    }

    if (!MvStyle::Font.Icon) {
        ImGui::TextWrapped(
            "Lucide font not loaded. Drop assets/fonts/lucide.ttf and rebuild.");
        ImGui::End();
        return;
    }

    static char filter[16] = "";
    ImGui::TextUnformatted("Filter (hex prefix, e.g. \"EA\"):");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("##filter", filter, sizeof(filter));
    ImGui::SameLine();
    if (ImGui::Button("Clear")) filter[0] = '\0';
    ImGui::SameLine();
    ImGui::TextDisabled("(click any glyph to copy codepoint)");
    ImGui::Separator();

    ImGui::BeginChild("##iconscroll", ImVec2(0, 0), false);

    ImFont* font = MvStyle::Font.Icon;
    constexpr int Cols = 12;
    int col = 0;

    auto encodeUtf8 = [](unsigned int cp, char* out) -> int {
        if (cp < 0x80) {
            out[0] = static_cast<char>(cp);
            out[1] = '\0';
            return 1;
        }
        if (cp < 0x800) {
            out[0] = static_cast<char>(0xC0 | (cp >> 6));
            out[1] = static_cast<char>(0x80 | (cp & 0x3F));
            out[2] = '\0';
            return 2;
        }
        out[0] = static_cast<char>(0xE0 | (cp >> 12));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
        out[3] = '\0';
        return 3;
    };

    // ImGui 1.92+ uses dynamic font baking — ImFont no longer exposes a
    // Glyphs vector. IsGlyphInFont() is the canonical "does this codepoint
    // exist in this font" check; we walk the full PUA range (6.4k codepoints,
    // O(1) per check) and skip everything that's missing. Acceptable for a
    // dev-only inspector.
    for (unsigned int cp = 0xE000; cp <= 0xF8FF; ++cp) {
        if (!font->IsGlyphInFont(static_cast<ImWchar>(cp))) continue;

        char hex[8];
        std::snprintf(hex, sizeof(hex), "%04X", cp);
        if (filter[0]) {
            // Case-insensitive prefix match
            bool match = true;
            for (int i = 0; filter[i] && match; ++i) {
                const char a = static_cast<char>(std::toupper(static_cast<unsigned char>(filter[i])));
                const char b = static_cast<char>(std::toupper(static_cast<unsigned char>(hex[i])));
                if (a != b) match = false;
            }
            if (!match) continue;
        }

        if (col > 0 && (col % Cols) != 0) ImGui::SameLine();
        ++col;

        ImGui::BeginGroup();
        ImGui::PushID(static_cast<int>(cp));

        char utf8[5];
        encodeUtf8(cp, utf8);

        ImGui::PushFont(font);
        if (ImGui::Button(utf8, ImVec2(40.0f, 40.0f))) {
            char clip[16];
            std::snprintf(clip, sizeof(clip), "0x%04X", cp);
            ImGui::SetClipboardText(clip);
        }
        ImGui::PopFont();

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("U+%04X — click to copy 0x%04X", cp, cp);
        }

        {
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
            const ImVec2 ts = ImGui::CalcTextSize(hex);
            const float pad = (40.0f - ts.x) * 0.5f;
            ImGui::Dummy(ImVec2(pad, 0.0f));
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::TextUnformatted(hex);
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
        ImGui::EndGroup();
    }

    if (col == 0) {
        ImGui::TextDisabled("No glyphs match the filter.");
    }

    ImGui::EndChild();
    ImGui::End();
}

void AppWindow::RenderAboutModal() {
    if (showAbout_) {
        ImGui::OpenPopup("About MetaVisage");
        showAbout_ = false;
    }
    if (ImGui::BeginPopupModal("About MetaVisage", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("MetaVisage");
        ImGui::Text("Version 2.0.0");
        ImGui::Separator();
        ImGui::TextWrapped("MetaHuman mesh morphing for Unreal Engine 5.x.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void AppWindow::RenderModals() {
    RenderAboutModal();
    RenderIconExplorer();
    RenderConfirmModal(confirmModal_);
    RenderErrorModal(errorModal_);
    RenderShortcutsModal(shortcutsModal_);

    RenderWelcomeModal(welcomeModal_);
    if (welcomeModal_.action != WelcomeAction::None) {
        switch (welcomeModal_.action) {
            case WelcomeAction::NewProject:  OnFileNewProject(); break;
            case WelcomeAction::OpenProject: pendingAction_ = PendingAction::OpenProject; break;
            case WelcomeAction::OpenRecent:  OpenProjectFromDisk(welcomeModal_.selectedRecent); break;
            case WelcomeAction::None: break;
        }
        welcomeModal_.action = WelcomeAction::None;
    }

    RenderExportModal(exportModal_);
    if (exportModal_.browseRequested) {
        exportModal_.browseRequested = false;
        // Format is selected via combo before Export is clicked; map the
        // formatIndex (0=FBX,1=OBJ,2=GLTF) to ExportFormat for the dialog
        // filter and the suggested filename extension.
        const auto fmt = static_cast<ExportFormat>(exportModal_.formatIndex);
        const std::string suggested = exportModal_.filepath.empty()
            ? std::string("export") + MeshExporter::GetFormatExtension(fmt)
            : exportModal_.filepath;
        if (auto picked = FileDialogs::SaveFile(ExportFiltersFor(fmt), suggested)) {
            exportModal_.filepath = *picked;
        }
    }
    if (exportModal_.action == ExportAction::Export) {
        exportModal_.action = ExportAction::None;
        if (exportModal_.filepath.empty()) {
            // Defensive fallback — the Browse button writes filepath, but if
            // the user submits with an empty field we still prompt before
            // bailing out so they're never silently sent back to the viewport.
            auto picked = FileDialogs::SaveFile(
                ExportFiltersFor(exportModal_.options.format),
                std::string("export") + MeshExporter::GetFormatExtension(exportModal_.options.format));
            if (picked) exportModal_.filepath = *picked;
        }
        if (!exportModal_.filepath.empty()) DoExport();
    }

    // Story 4.4 — Save / Rename Wrap preset modal. The render flips
    // `confirmed` on the frame the user clicks Save; we consume it here,
    // build the new (or updated) WrapPreset, persist it, and reload the
    // user-preset list so the sidebar's next frame picks up the change.
    RenderSavePresetModal(savePresetModal_);
    if (savePresetModal_.confirmed) {
        savePresetModal_.confirmed = false;
        if (project_) {
            const bool isRename = !savePresetModal_.editingPresetId.empty();
            WrapPreset preset;
            preset.id = isRename
                ? savePresetModal_.editingPresetId
                : PresetStore::GenerateUserPresetId();
            preset.name        = savePresetModal_.name;
            preset.description = savePresetModal_.description;
            preset.iconGlyph   = savePresetModal_.iconGlyph;
            preset.isBuiltIn   = false;
            preset.params      = isRename
                // Rename keeps the existing param snapshot; only metadata
                // changes. Find the original to preserve params.
                ? [&]() {
                    for (const auto& p : userPresets_) {
                        if (p.id == savePresetModal_.editingPresetId) return p.params;
                    }
                    return WrapParams::FromMorphData(project_->GetMorphData());
                  }()
                : WrapParams::FromMorphData(project_->GetMorphData());

            if (PresetStore::Save(preset)) {
                userPresets_ = PresetStore::LoadAll();
                statusMessage_ = isRename
                    ? std::format("Renamed preset to '{}'", preset.name)
                    : std::format("Saved preset '{}'", preset.name);
            } else {
                errorModal_.Open("Could not save preset to disk. "
                                 "Check %APPDATA%\\MetaVisage\\presets is writable.");
            }
        }
    }
}

bool AppWindow::HandleGlobalShortcut(int key, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return false;

    const bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (!ctrl) return false;

    // Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S / Ctrl+E / Ctrl+Z / Ctrl+Shift+Z
    // are handled here globally. Ctrl+E is stage-gated (Touch Up only) to
    // match the File menu's Export entry.
    switch (key) {
        case GLFW_KEY_N:
            if (!shift) { OnFileNewProject(); return true; }
            return false;
        case GLFW_KEY_O:
            if (!shift) { pendingAction_ = PendingAction::OpenProject; return true; }
            return false;
        case GLFW_KEY_S:
            pendingAction_ = shift ? PendingAction::SaveProjectAs
                                    : PendingAction::SaveProject;
            return true;
        case GLFW_KEY_E:
            if (!shift && project_ && project_->GetCurrentStage() == WorkflowStage::TouchUp) {
                pendingAction_ = PendingAction::ExportMesh;
                return true;
            }
            return false;
        case GLFW_KEY_Z:
            if (shift) OnEditRedo(); else OnEditUndo();
            return true;
        default:
            return false;
    }
}

void AppWindow::OnFileNewProject() {
    project_ = std::make_unique<Project>();
    WrapPreset::QuickWrap().ApplyTo(project_->GetMorphData());
    if (undoStack_) undoStack_->Clear();
    if (viewportHost_) viewportHost_->SetProject(project_.get());
    projectPath_.clear();
    statusMessage_ = "New project created";
    MV_LOG_INFO("File > New Project");
}

void AppWindow::OpenProjectFromDisk(const std::string& path) {
    if (path.empty()) return;
    auto fresh = std::make_unique<Project>();
    ProjectSerializer serializer;
    const SerializationResult result = serializer.Load(*fresh, path);
    if (!result.success) {
        errorModal_.Open(std::format("Could not open project:\n{}", result.errorMessage));
        return;
    }
    project_ = std::move(fresh);
    projectPath_ = path;
    project_->SetPath(path);

    // Reconstruct the deformed morph mesh from saved vertex/normal data —
    // ProjectSerializer parks the arrays in MorphData::savedDeformed*; the
    // actual `Mesh` object has to be assembled here so Stage 3/4 see a
    // populated `deformedMorphMesh` and the viewport renders the morph.
    MorphData& morphData = project_->GetMorphData();
    if (morphData.hasDeformedData && morphData.isProcessed &&
        project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {

        morphData.originalMorphMesh = project_->GetMorphMesh().mesh;

        auto deformed = std::make_shared<Mesh>();
        const Mesh& original = *project_->GetMorphMesh().mesh;
        deformed->SetVertices(morphData.savedDeformedVertices);
        deformed->SetNormals(morphData.savedDeformedNormals);
        deformed->SetUVs(original.GetUVs());
        deformed->SetFaces(original.GetFaces());
        deformed->SetMaterials(original.GetMaterials());
        deformed->SetName(original.GetName());

        morphData.deformedMorphMesh = deformed;
        morphData.savedDeformedVertices.clear();
        morphData.savedDeformedNormals.clear();
        morphData.hasDeformedData = false;
    }

    if (undoStack_) undoStack_->Clear();
    if (viewportHost_) viewportHost_->SetProject(project_.get());

    // Push the saved mask back onto the viewport so the heatmap reappears.
    // Without this the painted mask survives in MorphData but never reaches
    // the renderer until the user toggles Show Mask in the sidebar.
    if (viewportHost_ && morphData.deformedMorphMesh &&
        !morphData.vertexMask.empty()) {
        if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
            vp->SetMaskData(&morphData.vertexMask);
            vp->UploadMaskColors(morphData.deformedMorphMesh.get(),
                                 morphData.vertexMask);
        }
    }

    // Aim the camera at whatever mesh is on screen so a freshly loaded
    // project doesn't open with the model parked off-frame.
    if (viewportHost_) {
        const Mesh* focusMesh = morphData.deformedMorphMesh
            ? morphData.deformedMorphMesh.get()
            : (project_->GetMorphMesh().isLoaded ? project_->GetMorphMesh().mesh.get()
                                                  : nullptr);
        if (focusMesh) {
            const BoundingBox& bounds = focusMesh->GetBounds();
            if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
                vp->GetCamera()->FocusOnBounds(bounds);
            }
        }
    }

    ApplyStageViewportMode();
    Config::Instance().AddRecentProject(path);
    statusMessage_ = std::format("Opened {}", FileName(path));
    MV_LOG_INFO(std::format("Opened project {}", path));
}

void AppWindow::SaveProjectToDisk(const std::string& path) {
    if (!project_ || path.empty()) return;
    ProjectSerializer serializer;
    const SerializationResult result = serializer.Save(*project_, path);
    if (!result.success) {
        errorModal_.Open(std::format("Could not save project:\n{}", result.errorMessage));
        return;
    }
    projectPath_ = path;
    project_->SetPath(path);
    Config::Instance().AddRecentProject(path);
    statusMessage_ = std::format("Saved to {}", FileName(path));
    // Story 3.8.2 — drives the menu bar's "Saved Ns ago" indicator. Auto-save
    // (backlog) will set this from the timer thread; manual save is the
    // baseline source until then.
    lastSavedAt_ = std::chrono::steady_clock::now();
    MV_LOG_INFO(std::format("Saved project {}", path));
}

void AppWindow::ImportMorphMeshFromDisk(const std::string& path) {
    if (!project_ || path.empty()) return;
    MeshReference& ref = project_->GetMorphMesh();
    ref.mesh = std::make_shared<Mesh>();
    if (!ref.mesh->Load(path)) {
        ref.mesh.reset();
        ref.isLoaded = false;
        errorModal_.Open(std::format("Failed to load morph mesh:\n{}", path));
        return;
    }
    ref.filepath = path;
    ref.isLoaded = true;
    // Renderer lazy-uploads on first RenderMesh(); framing the bounds triggers
    // a paint and gives the user immediate visual feedback.
    if (viewportHost_ && viewportHost_->GetPrimaryViewport()) {
        viewportHost_->GetPrimaryViewport()->GetCamera()->FocusOnBounds(ref.mesh->GetBounds());
    }
    statusMessage_ = std::format("Loaded morph mesh: {}", FileName(path));
    MV_LOG_INFO(std::format("Loaded morph mesh: {}", path));
}

void AppWindow::ImportTargetMeshFromDisk(const std::string& path) {
    if (!project_ || path.empty()) return;
    MeshReference& ref = project_->GetTargetMesh();
    ref.mesh = std::make_shared<Mesh>();
    if (!ref.mesh->Load(path)) {
        ref.mesh.reset();
        ref.isLoaded = false;
        errorModal_.Open(std::format("Failed to load target mesh:\n{}", path));
        return;
    }
    ref.filepath = path;
    ref.isLoaded = true;
    if (viewportHost_ && viewportHost_->GetPrimaryViewport()) {
        viewportHost_->GetPrimaryViewport()->GetCamera()->FocusOnBounds(ref.mesh->GetBounds());
    }
    statusMessage_ = std::format("Loaded target mesh: {}", FileName(path));
    MV_LOG_INFO(std::format("Loaded target mesh: {}", path));
}

void AppWindow::UseDefaultMorphMesh() {
    if (!project_) return;
    const std::string exeDir = Paths::ExecutableDir();
    if (exeDir.empty()) {
        errorModal_.Open("Could not locate the application directory.");
        return;
    }
    const std::string path = exeDir + "/assets/models/MetaHumanHeadLod0.obj";
    if (!std::filesystem::exists(path)) {
        errorModal_.Open(std::format("Default MetaHuman head not found at:\n{}", path));
        return;
    }
    ImportMorphMeshFromDisk(path);
    // -90° Y-axis rotation orients the bundled head to face camera-forward.
    // Uses FromAxisAngle to side-step the param-order quirk in FromEulerAngles
    // (the second arg — documented as yaw/Y — produces a Z-axis rotation in
    // the current implementation).
    if (project_->GetMorphMesh().isLoaded) {
        project_->GetMorphMesh().transform.SetRotation(
            Quaternion::FromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), -90.0f));
    }
}

void AppWindow::ImportTextureForSlot(const std::string& slot) {
    if (!project_ || slot.empty()) return;
    std::vector<FileDialogs::Filter> filters = {
        {"Image (PNG, JPG, TGA, BMP)", "png,jpg,jpeg,tga,bmp"},
        {"PNG", "png"},
        {"JPEG", "jpg,jpeg"},
    };
    auto picked = FileDialogs::OpenFile(filters);
    if (!picked) return;
    auto tex = TextureData::LoadFromFile(*picked);
    if (!tex) {
        errorModal_.Open(std::format("Failed to load texture:\n{}", *picked));
        return;
    }
    project_->GetTargetTextures().Add(slot, tex);
    // Mirror Qt-era behaviour: only Albedo is uploaded as the active mesh
    // texture for preview; other slots are stored for baking/export use.
    if (slot == "Albedo" &&
        viewportHost_ && viewportHost_->GetPrimaryViewport() &&
        project_->GetTargetMesh().mesh) {
        viewportHost_->GetPrimaryViewport()->UploadMeshTexture(
            project_->GetTargetMesh().mesh.get(), *tex);
    }
    statusMessage_ = std::format("Loaded {} texture: {}", slot, FileName(*picked));
    MV_LOG_INFO(std::format("Loaded {} texture: {}", slot, *picked));
}

void AppWindow::ApplyStageViewportMode() {
    if (!viewportHost_ || !project_) return;
    const bool dual = project_->GetCurrentStage() == WorkflowStage::PointReference;
    viewportHost_->SetDualMode(dual);
}

void AppWindow::DoExport() {
    if (!project_ || exportModal_.filepath.empty()) return;
    auto& morph = project_->GetMorphData();

    // Mirror the Qt-era selection: prefer the deformed morph mesh, otherwise
    // fall back to the raw morph mesh so users can still export when they
    // skip the Morph stage (e.g. for sculpt-only edits).
    const Mesh* meshToExport = nullptr;
    if (morph.deformedMorphMesh) {
        meshToExport = morph.deformedMorphMesh.get();
    } else if (project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {
        meshToExport = project_->GetMorphMesh().mesh.get();
    }
    if (!meshToExport) {
        errorModal_.Open("No mesh available to export. Load or process a mesh first.");
        return;
    }

    const ExportOptions& options = exportModal_.options;
    const TextureSet& textures = project_->GetTargetTextures();
    const bool hasTextures = textures.HasAlbedo() || textures.HasNormalMap();
    const bool hasMorphedMesh = morph.deformedMorphMesh != nullptr;

    // Texture baking — runs before mesh export so the resulting albedo/normal
    // sit alongside the exported FBX/OBJ/GLTF for the user to pick up. Only
    // bakes when the user has both a morphed mesh and source textures, which
    // matches the Qt MainWindow path; otherwise the checkbox is disabled.
    if (options.exportBakedTextures && hasTextures && hasMorphedMesh &&
        project_->GetTargetMesh().isLoaded && project_->GetTargetMesh().mesh) {

        BakeSettings bakeSettings;
        bakeSettings.resolution = options.bakeResolution;
        bakeSettings.paddingPixels = 4;
        bakeSettings.bakeAlbedo = textures.HasAlbedo();
        bakeSettings.bakeNormalMap = textures.HasNormalMap();

        statusMessage_ = "Baking textures...";
        MV_LOG_INFO("Baking textures for export");

        TextureBaker baker;
        BakeResult bakeResult = baker.Bake(
            *morph.deformedMorphMesh,
            *project_->GetTargetMesh().mesh,
            project_->GetMorphMesh().transform,
            project_->GetTargetMesh().transform,
            textures.GetAlbedo().get(),
            textures.GetNormalMap() ? textures.GetNormalMap().get() : nullptr,
            bakeSettings);

        if (!bakeResult.success) {
            errorModal_.Open(std::format("Texture bake failed:\n{}", bakeResult.errorMessage));
            return;
        }

        // Write baked textures next to the mesh file using the convention
        // <basename>_albedo.<ext> / <basename>_normal.<ext>.
        const std::filesystem::path filePath(exportModal_.filepath);
        const std::string basePath = (filePath.parent_path() / filePath.stem()).string();
        const std::string ext = "." + options.textureFormat;

        if (bakeResult.bakedAlbedo) {
            const std::string albedoPath = basePath + "_albedo" + ext;
            const bool ok = (options.textureFormat == "tga")
                ? TextureWriter::WriteTGA(*bakeResult.bakedAlbedo, albedoPath)
                : TextureWriter::WritePNG(*bakeResult.bakedAlbedo, albedoPath);
            if (!ok) MV_LOG_ERROR(std::format("Failed to write baked albedo: {}", albedoPath));
        }
        if (bakeResult.bakedNormalMap) {
            const std::string normalPath = basePath + "_normal" + ext;
            const bool ok = (options.textureFormat == "tga")
                ? TextureWriter::WriteTGA(*bakeResult.bakedNormalMap, normalPath)
                : TextureWriter::WritePNG(*bakeResult.bakedNormalMap, normalPath);
            if (!ok) MV_LOG_ERROR(std::format("Failed to write baked normal: {}", normalPath));
        }
    }

    // For MetaHuman-compatible export, hand the original morph mesh to the
    // exporter so it can rewrite that file with deformed positions while
    // preserving the exact topology UE5 MetaHuman Conform requires.
    const Mesh* originalForCompat = nullptr;
    if (options.metaHumanCompatible && hasMorphedMesh &&
        project_->GetMorphMesh().isLoaded && project_->GetMorphMesh().mesh) {
        originalForCompat = project_->GetMorphMesh().mesh.get();
    } else {
        originalForCompat = morph.originalMorphMesh.get();
    }

    MeshExporter exporter;
    const ExportResult result = exporter.Export(
        *meshToExport,
        exportModal_.filepath,
        options,
        &project_->GetMorphMesh().transform,
        originalForCompat);
    if (!result.success) {
        errorModal_.Open(std::format("Export failed:\n{}", result.errorMessage));
        return;
    }
    statusMessage_ = std::format("Exported to {}", FileName(exportModal_.filepath));
    MV_LOG_INFO(std::format("Exported mesh to {}", exportModal_.filepath));
    // Visible confirmation matching v1.0 behaviour — the QMessageBox::information
    // toast in the Qt era turned into nothing on the ImGui port until now.
    errorModal_.OpenInfo(
        std::format("Mesh saved to:\n{}", exportModal_.filepath),
        "Export Complete");
}

void AppWindow::OnFileExit() {
    if (window_) glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void AppWindow::OnEditUndo() {
    if (!undoStack_ || !undoStack_->CanUndo()) return;
    undoStack_->Undo();
    // The action may have restored the workflow stage (e.g. undoing a
    // point placement done in Stage 2 while the user is now in Stage 3
    // takes them back to Stage 2). Sync the viewport host's dual-pane
    // state to match the new current stage.
    ApplyStageViewportMode();
    statusMessage_ = "Undo";
}

void AppWindow::OnEditRedo() {
    if (!undoStack_ || !undoStack_->CanRedo()) return;
    undoStack_->Redo();
    ApplyStageViewportMode();
    statusMessage_ = "Redo";
}

void AppWindow::RenderFrame(int /*fbWidth*/, int /*fbHeight*/) {
    // Debug HUD retained behind a single dockable window — useful during the
    // 3.6 rebuild. Removed before Sprint 3.6 closes.
    ImGui::SetNextWindowSize(ImVec2(320.0f, 110.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug HUD")) {
        ImGui::Text("Frame: %.3f ms (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::Text("GL:   %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        ImGui::Text("GLSL: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    }
    ImGui::End();
}

void AppWindow::Shutdown() {
    // Unwind in reverse init order, consulting each flag so a partial
    // Initialize() failure still produces a clean teardown.

    // Sprint 6 Story 6.3 -- join the licensing exchange worker so the
    // process doesn't exit while the POST is mid-flight. HttpClient has
    // a 30s ceiling; in practice the join is instant on a normal exit
    // because the user has already authenticated by the time they quit.
    LicenseManager::Instance().Shutdown();

    // Join the morph worker before tearing anything down — it holds raw
    // pointers into the deformer and captures `this` by reference.
    if (morphDeformer_) morphDeformer_->Cancel();
    if (morphThread_.joinable()) morphThread_.join();
    morphDeformer_.reset();

    if (viewportHost_) viewportHost_->SetProject(nullptr);
    undoStack_.reset();
    project_.reset();
    viewportHost_.reset();
    if (imguiOpenGLInitialized_) {
        // Free GL textures owned by MvStyle (logo) before tearing down the
        // OpenGL backend — the GL context is still current here.
        MvStyle::Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        imguiOpenGLInitialized_ = false;
    }
    if (imguiGlfwInitialized_) {
        ImGui_ImplGlfw_Shutdown();
        imguiGlfwInitialized_ = false;
    }
    if (imguiContextCreated_) {
        ImGui::DestroyContext();
        imguiContextCreated_ = false;
    }
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

// ---------------------------------------------------------------------------
// Stage 3 — Morph worker lifecycle
// ---------------------------------------------------------------------------

void AppWindow::StartMorph() {
    if (!project_) return;
    if (morphRunning_.load()) return;

    // Join any stale thread from a prior run before reusing the handle.
    if (morphThread_.joinable()) morphThread_.join();

    MorphData& morphData = project_->GetMorphData();
    MeshReference& morphMeshRef = project_->GetMorphMesh();
    MeshReference& targetMeshRef = project_->GetTargetMesh();

    if (!morphMeshRef.isLoaded || !morphMeshRef.mesh ||
        !targetMeshRef.isLoaded || !targetMeshRef.mesh) {
        errorModal_.Open("Both meshes must be loaded before processing.",
                         "Morph Failed");
        return;
    }

    // Capture the untouched morph mesh so repeat runs operate on the original
    // geometry rather than compounding deformations.
    if (!morphData.originalMorphMesh) {
        morphData.originalMorphMesh = morphMeshRef.mesh;
    }
    if (morphData.deformedMorphMesh && viewportHost_) {
        if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
            vp->InvalidateMesh(morphData.deformedMorphMesh.get());
        }
        if (Viewport* vp = viewportHost_->GetSecondaryViewport()) {
            vp->InvalidateMesh(morphData.deformedMorphMesh.get());
        }
    }
    // PR #20 review blocker: SculptStrokeUndoAction stores a raw Mesh* into
    // morphData.deformedMorphMesh.get(). PollMorphCompletion will replace that
    // shared_ptr when this morph finishes — destroying the Mesh those undo
    // actions point at. Clearing the stack here matches the discipline
    // OnFileNewProject and OpenProjectFromDisk already use, and is the same
    // pattern the Qt-era MainWindow::OnMorphPreviewModeChanged followed.
    if (undoStack_) undoStack_->Clear();
    morphData.isProcessed = false;
    morphData.isAccepted  = false;

    NRICPParams params;
    params.stiffnessSteps          = morphData.nricpStiffnessSteps;
    params.alphaInitial            = morphData.nricpAlphaInitial;
    params.alphaFinal              = morphData.nricpAlphaFinal;
    params.icpIterations           = morphData.nricpIcpIterations;
    params.normalThreshold         = morphData.nricpNormalThreshold;
    params.landmarkWeight          = morphData.nricpLandmarkWeight;
    params.epsilon                 = morphData.nricpEpsilon;
    params.enableBoundaryExclusion = morphData.nricpEnableBoundaryExclusion;
    params.boundaryExclusionHops   = morphData.nricpBoundaryExclusionHops;
    params.optimizationIterations  = morphData.nricpOptimizationIterations;
    params.dpInitial               = morphData.nricpDpInitial;
    params.dpFinal                 = morphData.nricpDpFinal;
    params.gammaInitial            = morphData.nricpGammaInitial;
    params.gammaFinal              = morphData.nricpGammaFinal;
    params.samplingInitial         = morphData.nricpSamplingInitial;
    params.samplingFinal           = morphData.nricpSamplingFinal;
    params.normalizeSampling       = morphData.nricpNormalizeSampling;

    morphDeformer_ = std::make_unique<MeshDeformer>();
    morphDeformer_->SetSourceMesh(morphData.originalMorphMesh);
    morphDeformer_->SetTargetMesh(targetMeshRef.mesh);
    morphDeformer_->SetMorphTransform(morphMeshRef.transform);
    morphDeformer_->SetTargetTransform(targetMeshRef.transform);
    morphDeformer_->SetCorrespondences(project_->GetPointReferenceData().correspondences);
    morphDeformer_->SetStiffness(morphData.stiffness);
    morphDeformer_->SetSmoothness(morphData.smoothness);
    morphDeformer_->SetNRICPParams(params);
    if (!morphData.vertexMask.empty()) {
        morphDeformer_->SetUserExcludedVertices(morphData.vertexMask);
    }
    morphDeformer_->SetProgressCallback([this](float progress, const std::string&) {
        morphProgress_.store(progress);
    });

    morphProgress_.store(0.0f);
    morphFinished_.store(false);
    morphRunning_.store(true);

    morphThread_ = std::thread([this]() {
        DeformationResult result = morphDeformer_->Deform();

        {
            std::lock_guard<std::mutex> lock(morphResultMutex_);
            morphResultSuccess_      = result.success;
            morphResultError_        = result.errorMessage;
            morphResultMesh_         = result.deformedMesh;
            morphResultDisplacements_ = morphDeformer_->GetDisplacementMagnitudes();
            morphResultMaxDisp_      = result.maxDisplacement;
            morphResultAvgDisp_      = result.avgDisplacement;
        }
        morphFinished_.store(true);
    });
}

void AppWindow::CancelMorph() {
    if (morphDeformer_ && morphRunning_.load()) {
        morphDeformer_->Cancel();
    }
}

void AppWindow::PollMorphCompletion() {
    if (!morphFinished_.load()) return;

    if (morphThread_.joinable()) morphThread_.join();
    morphRunning_.store(false);
    morphFinished_.store(false);

    if (!project_) {
        morphResultMesh_.reset();
        return;
    }

    bool success;
    std::string error;
    std::shared_ptr<Mesh> deformedMesh;
    std::vector<float> displacements;
    float maxDisp, avgDisp;
    {
        std::lock_guard<std::mutex> lock(morphResultMutex_);
        success       = morphResultSuccess_;
        error         = morphResultError_;
        deformedMesh  = std::move(morphResultMesh_);
        displacements = std::move(morphResultDisplacements_);
        maxDisp       = morphResultMaxDisp_;
        avgDisp       = morphResultAvgDisp_;
    }

    MorphData& morphData = project_->GetMorphData();
    if (success && deformedMesh) {
        morphData.deformedMorphMesh      = deformedMesh;
        morphData.displacementMagnitudes = displacements;
        morphData.maxDisplacement        = maxDisp;
        morphData.avgDisplacement        = avgDisp;
        morphData.isProcessed            = true;
        morphData.isAccepted             = false;

        if (viewportHost_) {
            if (Viewport* vp = viewportHost_->GetPrimaryViewport()) {
                vp->UploadHeatMapColors(deformedMesh.get(), displacements, maxDisp);
            }
        }
    } else {
        errorModal_.Open(error.empty() ? std::string("Processing cancelled or failed.") : error,
                         "Morph Failed");
    }

    morphDeformer_.reset();
}

void AppWindow::AcceptMorph() {
    if (!project_) return;
    MorphData& morphData = project_->GetMorphData();
    if (!morphData.isProcessed) return;
    morphData.isAccepted = true;
}

} // namespace MetaVisage
