#include "ui/StageSidebar.h"
#include "core/Project.h"
#include "core/TextureData.h"

#include <imgui.h>

#include <QFileInfo>
#include <array>

namespace MetaVisage {

namespace {

const char* StageName(WorkflowStage s) {
    switch (s) {
        case WorkflowStage::Alignment:      return "Alignment";
        case WorkflowStage::PointReference: return "Point Reference";
        case WorkflowStage::Morph:          return "Morph";
        case WorkflowStage::TouchUp:        return "Touch Up";
    }
    return "";
}

const char* StageHint(WorkflowStage s) {
    switch (s) {
        case WorkflowStage::Alignment:
            return "Position the target mesh to roughly align with the morph mesh.";
        case WorkflowStage::PointReference:
            return "Place matching points on each mesh to guide deformation.";
        case WorkflowStage::Morph:
            return "Tune parameters and process. Review before accepting.";
        case WorkflowStage::TouchUp:
            return "Sculpt with brushes to refine the result, then export.";
    }
    return "";
}

QString FileNameOrEmpty(const QString& path) {
    if (path.isEmpty()) return {};
    return QFileInfo(path).fileName();
}

int AxisToIndex(Axis a) {
    switch (a) { case Axis::X: return 0; case Axis::Y: return 1; case Axis::Z: return 2; }
    return 0;
}

Axis IndexToAxis(int i) {
    switch (i) { case 0: return Axis::X; case 1: return Axis::Y; case 2: return Axis::Z; }
    return Axis::X;
}

} // namespace

StageSidebar::Actions StageSidebar::Render(Project* project,
                                           bool morphProcessing,
                                           float morphProgress) {
    Actions out;

    // NoMove + NoCollapse + NoTitleBar work together with the dock node flags
    // set by AppWindow (NoUndocking / NoDockingOverMe / HiddenTabBar) to keep
    // the sidebar statically pinned to the right of the app shell.
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar;
    if (!ImGui::Begin("Stage", nullptr, flags)) { ImGui::End(); return out; }

    if (!project) {
        ImGui::TextDisabled("No project loaded.");
        ImGui::End();
        return out;
    }

    const WorkflowStage stage = project->GetCurrentStage();
    ImGui::Text("Stage %d / 4", static_cast<int>(stage) + 1);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.85f, 1.0f, 1.0f));
    ImGui::TextUnformatted(StageName(stage));
    ImGui::PopStyleColor();
    ImGui::TextWrapped("%s", StageHint(stage));
    ImGui::Separator();

    // Per-stage body
    switch (stage) {
        case WorkflowStage::Alignment:      RenderAlignment(*project, out); break;
        case WorkflowStage::PointReference: RenderPointReference(*project, out); break;
        case WorkflowStage::Morph:          RenderMorph(*project, morphProcessing, morphProgress, out); break;
        case WorkflowStage::TouchUp:        RenderTouchUp(*project, out); break;
    }

    // Stage navigation footer
    ImGui::Separator();
    const bool canGoBack = (static_cast<int>(stage) > 0);
    ImGui::BeginDisabled(!canGoBack);
    if (ImGui::Button("< Previous")) out.previousStageRequested = true;
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool canGoNext = (static_cast<int>(stage) < static_cast<int>(WorkflowStage::TouchUp))
                         && project->CanProceedToNextStage();
    ImGui::BeginDisabled(!canGoNext);
    if (ImGui::Button("Next >")) out.nextStageRequested = true;
    ImGui::EndDisabled();

    ImGui::End();
    return out;
}

// ---------------------------------------------------------------------------
// Stage 1 — Alignment
// ---------------------------------------------------------------------------

void StageSidebar::RenderAlignment(Project& project, Actions& out) {
    auto& morph = project.GetMorphMesh();
    auto& target = project.GetTargetMesh();

    ImGui::TextUnformatted("Morph Mesh");
    if (morph.filepath.isEmpty()) {
        ImGui::TextDisabled("(not set)");
    } else {
        ImGui::TextWrapped("%s", FileNameOrEmpty(morph.filepath).toUtf8().constData());
    }
    if (ImGui::Button("Import Morph Mesh...")) out.importMorphMeshRequested = true;
    ImGui::SameLine();
    if (ImGui::Button("Use Default MetaHuman Head")) out.useDefaultMorphMeshRequested = true;

    ImGui::Spacing();
    ImGui::TextUnformatted("Target Mesh");
    if (target.filepath.isEmpty()) {
        ImGui::TextDisabled("(not set)");
    } else {
        ImGui::TextWrapped("%s", FileNameOrEmpty(target.filepath).toUtf8().constData());
    }
    if (ImGui::Button("Import Target Mesh...")) out.importTargetMeshRequested = true;

    ImGui::Spacing();
    ImGui::TextUnformatted("Target Textures");
    ImGui::Separator();
    auto& textures = project.GetTargetTextures();
    if (textures.textures.empty()) {
        ImGui::TextDisabled("(no textures)");
    } else {
        for (const auto& t : textures.textures) {
            ImGui::BulletText("%s", t.name.toUtf8().constData());
        }
    }
    static const char* kSlotNames[] = {
        "Albedo", "Normal Map", "Roughness", "AO", "Emissive", "Metallic"
    };
    static int sSelectedSlot = 0;
    ImGui::PushItemWidth(140);
    ImGui::Combo("##texSlot", &sSelectedSlot, kSlotNames, IM_ARRAYSIZE(kSlotNames));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("+ Add Texture")) {
        out.importTextureSlot = QString::fromUtf8(kSlotNames[sSelectedSlot]);
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Target Transform");
    ImGui::Separator();

    // Target mesh transform is owned by MeshReference::transform — that's what
    // Renderer/Viewport read at draw + raycast time. AlignmentData has a shadow
    // copy that's only used by the serializer round-trip; writing there alone
    // would leave the on-screen mesh unchanged.
    Transform& xform = project.GetTargetMesh().transform;
    float pos[3] = { xform.GetPosition().x, xform.GetPosition().y, xform.GetPosition().z };
    if (ImGui::DragFloat3("Position", pos, 0.01f)) {
        xform.SetPosition(Vector3(pos[0], pos[1], pos[2]));
    }
    Vector3 euler = xform.GetEulerAngles();
    float rot[3] = { euler.x, euler.y, euler.z };
    if (ImGui::DragFloat3("Rotation", rot, 0.5f, -360.0f, 360.0f)) {
        xform.SetRotationFromEuler(rot[0], rot[1], rot[2]);
    }
    float scl[3] = { xform.GetScale().x, xform.GetScale().y, xform.GetScale().z };
    if (ImGui::DragFloat3("Scale", scl, 0.01f, 0.001f, 1000.0f)) {
        xform.SetScale(Vector3(scl[0], scl[1], scl[2]));
    }
    if (ImGui::Button("Reset Transform")) out.resetTransformRequested = true;

}

// ---------------------------------------------------------------------------
// Stage 2 — Point Reference
// ---------------------------------------------------------------------------

void StageSidebar::RenderPointReference(Project& project, Actions& out) {
    auto& data = project.GetPointReferenceData();
    int paired = 0;
    int morphOnly = 0;
    int targetOnly = 0;
    for (const auto& c : data.correspondences) {
        const bool hasM = c.morphMeshVertexIndex >= 0;
        const bool hasT = c.targetMeshVertexIndex >= 0;
        if (hasM && hasT) ++paired;
        else if (hasM) ++morphOnly;
        else if (hasT) ++targetOnly;
    }
    ImGui::Text("Paired points:     %d", paired);
    ImGui::Text("Morph-only:        %d", morphOnly);
    ImGui::Text("Target-only:       %d", targetOnly);
    ImGui::Spacing();

    bool symmetry = data.symmetryEnabled;
    if (ImGui::Checkbox("Mirror across symmetry axis", &symmetry)) {
        data.symmetryEnabled = symmetry;
    }
    int axisIdx = AxisToIndex(data.symmetryAxis);
    const char* axes[] = {"X", "Y", "Z"};
    ImGui::BeginDisabled(!symmetry);
    if (ImGui::Combo("Symmetry Axis", &axisIdx, axes, 3)) {
        data.symmetryAxis = IndexToAxis(axisIdx);
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::SliderFloat("Point Size", &data.pointRenderSize, 4.0f, 32.0f, "%.1f");

    ImGui::Spacing();
    if (ImGui::Button("Clear All Points")) out.clearAllPointsRequested = true;
}

// ---------------------------------------------------------------------------
// Stage 3 — Morph
// ---------------------------------------------------------------------------

void StageSidebar::RenderMorph(Project& project, bool processing, float progress, Actions& out) {
    auto& morph = project.GetMorphData();

    // Sprint 4 will reintroduce the Quick Wrap / Fantasy Wrap preset cards.
    // For now we expose the raw parameter fields that already exist on
    // MorphData so the pipeline can be tuned manually.
    ImGui::TextUnformatted("Parameters");
    ImGui::Separator();

    ImGui::SliderInt("Stiffness Steps",     &morph.nricpStiffnessSteps, 1, 10);
    ImGui::SliderFloat("Stiffness (Start)", &morph.nricpAlphaInitial,   1.0f, 500.0f, "%.1f");
    ImGui::SliderFloat("Stiffness (End)",   &morph.nricpAlphaFinal,     0.1f, 200.0f, "%.1f");
    ImGui::SliderInt("Iterations / Step",   &morph.nricpIcpIterations,  1, 20);
    ImGui::SliderFloat("Normal Threshold",  &morph.nricpNormalThreshold, 5.0f, 120.0f, "%.0f°");
    ImGui::SliderFloat("Landmark Weight",   &morph.nricpLandmarkWeight, 0.0f, 100.0f, "%.1f");

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::SliderFloat("Control Density (Start)", &morph.nricpSamplingInitial, 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Control Density (End)",   &morph.nricpSamplingFinal,   0.0f, 1.0f, "%.3f");
        ImGui::Checkbox("Normalize Sampling",         &morph.nricpNormalizeSampling);

        ImGui::SliderFloat("Rigidity (Start)",        &morph.nricpGammaInitial,    0.0f, 5.0f, "%.2f");
        ImGui::SliderFloat("Rigidity (End)",          &morph.nricpGammaFinal,      0.0f, 5.0f, "%.2f");

        ImGui::SliderFloat("Step Damping (Start)",    &morph.nricpDpInitial,       0.1f, 1.0f, "%.2f");
        ImGui::SliderFloat("Step Damping (End)",      &morph.nricpDpFinal,         0.1f, 1.0f, "%.2f");

        ImGui::SliderInt("Inner Iterations",          &morph.nricpOptimizationIterations, 1, 10);
        ImGui::InputFloat("Convergence Threshold",    &morph.nricpEpsilon, 0.0f, 0.0f, "%.5f");

        ImGui::Checkbox("Exclude Mesh Boundaries",    &morph.nricpEnableBoundaryExclusion);
        ImGui::BeginDisabled(!morph.nricpEnableBoundaryExclusion);
        ImGui::SliderInt("Boundary Hops",             &morph.nricpBoundaryExclusionHops, 0, 10);
        ImGui::EndDisabled();

        if (ImGui::Button("Reset to Defaults", ImVec2(-FLT_MIN, 0))) {
            morph = MorphData();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (processing) {
        char label[64];
        std::snprintf(label, sizeof(label), "Processing… %.0f%%", progress * 100.0f);
        ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0), label);
        if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 0))) out.cancelMorphRequested = true;
    } else {
        const bool canProcess = project.GetMorphMesh().isLoaded
                             && project.GetTargetMesh().isLoaded;
        ImGui::BeginDisabled(!canProcess);
        if (ImGui::Button("Process Morph", ImVec2(-FLT_MIN, 0))) out.processMorphRequested = true;
        ImGui::EndDisabled();
        if (!canProcess) ImGui::TextDisabled("Load both meshes before processing.");
    }

    ImGui::Spacing();
    if (morph.isProcessed) {
        ImGui::Text("Max displacement: %.3f", morph.maxDisplacement);
        ImGui::Text("Avg displacement: %.3f", morph.avgDisplacement);

        static int previewIdx = static_cast<int>(MorphPreviewMode::Deformed);
        const char* modes[] = {"Deformed", "Original", "Overlay", "Heat Map"};
        if (ImGui::Combo("Preview", &previewIdx, modes, 4)) {
            morph.previewMode = static_cast<MorphPreviewMode>(previewIdx);
        }

        ImGui::BeginDisabled(morph.isAccepted);
        if (ImGui::Button("Accept & Continue", ImVec2(-FLT_MIN, 0))) out.acceptMorphRequested = true;
        ImGui::EndDisabled();
    }

    // ------------------------------------------------------------------
    // Mask painting — available any time a morph-side mesh exists (before
    // or after processing). Painted vertices are excluded from the NRICP
    // correspondence search on the next Process, letting the user pin eyes
    // / mouths / interior cavities that would otherwise pull wrong.
    // ------------------------------------------------------------------
    const bool morphMeshReady = project.GetMorphMesh().isLoaded || morph.isProcessed;
    if (morphMeshReady) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Vertex Mask");
        ImGui::TextDisabled("Paint to exclude vertices from the next Process.");

        ImGui::Checkbox("Paint Mask", &maskPaintingActive_);
        ImGui::SameLine();
        ImGui::Checkbox("Show Mask", &maskShowMask_);

        ImGui::BeginDisabled(!maskPaintingActive_);
        ImGui::SliderFloat("Mask Radius", &maskBrushRadius_, 0.5f, 50.0f, "%.1f");
        ImGui::Checkbox("Erase", &maskEraseMode_);
        ImGui::EndDisabled();

        const bool hasMaskData = !morph.vertexMask.empty();
        ImGui::BeginDisabled(!hasMaskData);
        if (ImGui::Button("Clear Mask")) out.clearMaskRequested = true;
        ImGui::SameLine();
        if (ImGui::Button("Invert Mask")) out.invertMaskRequested = true;
        ImGui::EndDisabled();

        // Push current mask UI state through Actions so AppWindow can mirror
        // onto the primary viewport each frame.
        out.maskPaintingActive = maskPaintingActive_;
        out.maskShow           = maskShowMask_ || maskPaintingActive_;
        out.maskEraseMode      = maskEraseMode_;
        out.maskBrushRadius    = maskBrushRadius_;
    } else {
        // Ensure viewport does not treat LMB as mask painting when no mesh is
        // loaded yet.
        out.maskPaintingActive = false;
        out.maskShow           = false;
    }
}

// ---------------------------------------------------------------------------
// Stage 4 — Touch Up
// ---------------------------------------------------------------------------

void StageSidebar::RenderTouchUp(Project& project, Actions& out) {
    auto& sculpt = project.GetSculptState();

    ImGui::TextUnformatted("Brush");
    ImGui::Separator();
    static const std::array<std::pair<BrushType, const char*>, 4> kBrushes = {{
        {BrushType::Smooth,   "Smooth"},
        {BrushType::Grab,     "Grab"},
        {BrushType::PushPull, "Push/Pull"},
        {BrushType::Inflate,  "Inflate"},
    }};
    for (size_t i = 0; i < kBrushes.size(); ++i) {
        const bool selected = (sculpt.activeBrush == kBrushes[i].first);
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(kBrushes[i].second, selected)) {
            sculpt.activeBrush = kBrushes[i].first;
        }
    }

    ImGui::Spacing();
    ImGui::SliderFloat("Radius",   &sculpt.brushRadius,   1.0f, 100.0f, "%.1f");
    ImGui::SliderFloat("Strength", &sculpt.brushStrength, 0.0f, 1.0f,   "%.2f");

    int falloffIdx = static_cast<int>(sculpt.falloff);
    const char* falloffs[] = {"Linear", "Smooth", "Sharp"};
    if (ImGui::Combo("Falloff", &falloffIdx, falloffs, 3)) {
        sculpt.falloff = static_cast<FalloffType>(falloffIdx);
    }

    ImGui::Spacing();
    ImGui::Checkbox("Symmetry", &sculpt.symmetryEnabled);
    ImGui::SameLine();
    int axisIdx = AxisToIndex(sculpt.symmetryAxis);
    const char* axes[] = {"X", "Y", "Z"};
    ImGui::BeginDisabled(!sculpt.symmetryEnabled);
    ImGui::PushItemWidth(80);
    if (ImGui::Combo("##sym_axis", &axisIdx, axes, 3)) {
        sculpt.symmetryAxis = IndexToAxis(axisIdx);
    }
    ImGui::PopItemWidth();
    ImGui::EndDisabled();

    ImGui::Checkbox("Show target overlay", &sculpt.showTargetOverlay);

    ImGui::Spacing();
    ImGui::Separator();
    // "Export Mesh..." is the only export entry point now — the v1.0 era
    // "Finalize" button was just a wrapper that ran a mesh-validation popup
    // and then called the same export flow, which created two redundant
    // buttons in the sidebar. Mesh validation can move into the export
    // pipeline itself if we want it back.
    if (ImGui::Button("Export Mesh...", ImVec2(-FLT_MIN, 0))) out.exportMeshRequested = true;
}

} // namespace MetaVisage
