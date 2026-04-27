#include "ui/AppWindow.h"
#include "ui/FileDialogs.h"
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

namespace MetaVisage {

namespace {

void GlfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
    MV_LOG_ERROR(std::format("GLFW error {}: {}", error, description));
}

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

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        MV_LOG_CRITICAL("glfwCreateWindow failed");
        glfwTerminate();
        return false;
    }

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

    ApplyDarkTheme();

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

    MV_LOG_INFO(std::format("AppWindow initialized: {}x{}, GL {}, GLSL {}",
        width, height,
        reinterpret_cast<const char*>(glGetString(GL_VERSION)),
        reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION))));

    viewportHost_ = std::make_unique<ViewportHost>();
    if (!viewportHost_->Initialize()) {
        MV_LOG_CRITICAL("ViewportHost initialization failed");
        viewportHost_.reset();
        Shutdown();
        return false;
    }

    project_ = std::make_unique<Project>();
    undoStack_ = std::make_unique<UndoStack>();
    viewportHost_->SetProject(project_.get());
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
    auto onPointDelete = [this]() {
        if (!project_ || !viewportHost_) return;
        auto* vp = viewportHost_->GetPrimaryViewport();
        int primaryIdx = vp ? vp->GetSelectedPointIndex() : -1;
        int secondaryIdx = viewportHost_->GetSecondaryViewport()
            ? viewportHost_->GetSecondaryViewport()->GetSelectedPointIndex()
            : -1;
        int selected = (primaryIdx >= 0) ? primaryIdx : secondaryIdx;
        if (selected < 0) return;

        auto& correspondences = project_->GetPointReferenceData().correspondences;
        if (selected >= static_cast<int>(correspondences.size())) return;

        const std::vector<PointCorrespondence> beforeSnapshot = correspondences;

        int pairID = -1;
        if (correspondences[selected].isSymmetric) {
            pairID = correspondences[selected].symmetricPairID;
        }
        correspondences.erase(correspondences.begin() + selected);
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

void AppWindow::ApplyDarkTheme() {
    // Mirror the v1.0 Qt Fusion dark palette so ImGui doesn't flash a lighter
    // theme. Exact colors match src/main.cpp's pre-migration palette.
    ImGui::StyleColorsDark();

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 4.0f;
    s.FrameRounding = 3.0f;
    s.GrabRounding = 3.0f;

    auto rgb = [](int r, int g, int b, float a = 1.0f) {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
    };

    s.Colors[ImGuiCol_WindowBg]       = rgb(30, 30, 30);
    s.Colors[ImGuiCol_ChildBg]        = rgb(30, 30, 30);
    s.Colors[ImGuiCol_PopupBg]        = rgb(44, 62, 80);
    s.Colors[ImGuiCol_FrameBg]        = rgb(44, 62, 80);
    s.Colors[ImGuiCol_FrameBgHovered] = rgb(52, 73, 94);
    s.Colors[ImGuiCol_FrameBgActive]  = rgb(52, 152, 219);
    s.Colors[ImGuiCol_TitleBg]        = rgb(30, 30, 30);
    s.Colors[ImGuiCol_TitleBgActive]  = rgb(44, 62, 80);
    s.Colors[ImGuiCol_MenuBarBg]      = rgb(35, 40, 45);
    s.Colors[ImGuiCol_Button]         = rgb(44, 62, 80);
    s.Colors[ImGuiCol_ButtonHovered]  = rgb(52, 73, 94);
    s.Colors[ImGuiCol_ButtonActive]   = rgb(52, 152, 219);
    s.Colors[ImGuiCol_Header]         = rgb(44, 62, 80);
    s.Colors[ImGuiCol_HeaderHovered]  = rgb(52, 73, 94);
    s.Colors[ImGuiCol_HeaderActive]   = rgb(52, 152, 219);
    s.Colors[ImGuiCol_CheckMark]      = rgb(52, 152, 219);
    s.Colors[ImGuiCol_SliderGrab]     = rgb(52, 152, 219);
    s.Colors[ImGuiCol_SliderGrabActive] = rgb(41, 128, 185);
    s.Colors[ImGuiCol_Text]           = rgb(255, 255, 255);
    s.Colors[ImGuiCol_TextDisabled]   = rgb(127, 140, 141);
}

void AppWindow::Run() {
    if (!window_) return;

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // App shell (dockspace + menu bar + status bar) goes first so that
        // docked windows — the future sidebar and the viewport — can reserve
        // their dock slots before BuildLayout runs.
        RenderAppShell();

        if (viewportHost_) {
            viewportHost_->BuildLayout(
                fbWidth, fbHeight,
                centralRectPos_.x, centralRectPos_.y,
                centralRectSize_.x, centralRectSize_.y);
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
            auto actions = stageSidebar_.Render(project_.get(),
                                                morphRunning_.load(),
                                                morphProgress_.load());

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

        ImGui::Render();

        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(30.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (viewportHost_) viewportHost_->RenderGL(fbHeight);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
    }
}

void AppWindow::RenderAppShell() {
    // Master host-window pattern: a single borderless window spans the main
    // viewport, contains the menu bar, reserves a status-bar strip at the
    // bottom, and hosts an ImGui DockSpace in between. Sidebar/viewport/etc.
    // dock into that space by matching the dock ID on Begin().
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
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

    RenderMainMenuBar();

    // Reserve a row for the status bar at the bottom of the host window.
    const float statusH = ImGui::GetFrameHeight();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 dockSize(avail.x, avail.y - statusH);

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

    ImGui::End();
}

void AppWindow::RenderMainMenuBar() {
    if (!ImGui::BeginMenuBar()) return;

    const WorkflowStage stage = project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;
    const bool inTouchUp = (stage == WorkflowStage::TouchUp);
    const bool canUndo = undoStack_ && undoStack_->CanUndo();
    const bool canRedo = undoStack_ && undoStack_->CanRedo();

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project", "Ctrl+N")) OnFileNewProject();
        if (ImGui::MenuItem("Open Project...", "Ctrl+O")) pendingAction_ = PendingAction::OpenProject;
        ImGui::Separator();
        // Save becomes Save As until we persist a project path (Sprint 3.6.4).
        if (ImGui::MenuItem("Save Project", "Ctrl+S")) pendingAction_ = PendingAction::SaveProject;
        if (ImGui::MenuItem("Save Project As...", "Ctrl+Shift+S")) pendingAction_ = PendingAction::SaveProjectAs;
        ImGui::Separator();
        // Import Morph Mesh / Import Target Mesh / Export Mesh are stage-gated
        // workflows — the sidebar surfaces them in the stages where they make
        // sense (Stage 1 for imports, Stage 4 for export). Mirroring them in
        // the File menu duplicated entry points and confused users about when
        // they apply, so the menu entries were removed.
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
        // Shading mode lives on the viewport overlay (top-right combo) where
        // it's adjusted directly while looking at the model — duplicating it
        // in the menu just gave users two places to set the same thing.
        // Grid toggle and camera-view presets remain stubbed for 3.6.5.
        ImGui::MenuItem("Show Grid", nullptr, false, false);
        ImGui::Separator();
        ImGui::MenuItem("Front View (Numpad 1)", nullptr, false, false);
        ImGui::MenuItem("Right View (Numpad 3)", nullptr, false, false);
        ImGui::MenuItem("Top View (Numpad 7)", nullptr, false, false);
        ImGui::MenuItem("Perspective (Home)", nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Documentation")) pendingAction_ = PendingAction::OpenDocumentation;
        if (ImGui::MenuItem("Account & License")) pendingAction_ = PendingAction::OpenAccountLicense;
        if (ImGui::MenuItem("Check for Updates...")) pendingAction_ = PendingAction::CheckForUpdates;
        ImGui::Separator();
        if (ImGui::MenuItem("Keyboard Shortcuts")) pendingAction_ = PendingAction::ShowKeyboardShortcuts;
        if (ImGui::MenuItem("About MetaVisage")) pendingAction_ = PendingAction::ShowAbout;
        ImGui::EndMenu();
    }

    // Right-side stage indicator — mirrors the sidebar's `N / 4` element but
    // is always visible even before the sidebar is docked. Removed once the
    // sidebar panel lands in 3.6.2.
    const char* stageName = StageDisplayName(stage);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Stage %d / 4 — %s",
                  static_cast<int>(stage) + 1, stageName);
    float textW = ImGui::CalcTextSize(buf).x;
    float available = ImGui::GetContentRegionAvail().x;
    if (available > textW + 16.0f) {
        ImGui::SameLine(ImGui::GetCursorPosX() + available - textW - 8.0f);
        ImGui::TextUnformatted(buf);
    }

    ImGui::EndMenuBar();
}

void AppWindow::RenderStatusBar() {
    // Status bar is the final row of the host window. Keep it single-line and
    // subdued — detailed tool / hint / tier-badge rendering lands as the
    // sidebar and licensing subsystems come online.
    ImGui::Separator();
    const WorkflowStage stage = project_ ? project_->GetCurrentStage() : WorkflowStage::Alignment;
    ImGui::Text("Stage %d / 4 — %s  |  %s",
                static_cast<int>(stage) + 1,
                StageDisplayName(stage),
                statusMessage_.c_str());
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
    morphDeformer_->SetKernelType(morphData.algorithm);
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
