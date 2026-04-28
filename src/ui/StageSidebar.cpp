#include "ui/StageSidebar.h"
#include "core/Mesh.h"
#include "core/Project.h"
#include "core/TextureData.h"
#include "ui/MvIcons.h"
#include "ui/MvInspectorPrimitives.h"
#include "ui/MvStyle.h"

#include <imgui.h>

#include <cstdio>

#include <algorithm>
#include <array>
#include <filesystem>

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

std::string FileNameOrEmpty(const std::string& path) {
    if (path.empty()) return {};
    return std::filesystem::path(path).filename().string();
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
                                           float morphProgress,
                                           int selectedPointIndex) {
    Actions out;
    selectedPointIndexExternal_ = selectedPointIndex;

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

    // Story 3.8.7+: per-stage panels own their full chrome (header,
    // sectioned body, footer). Stages migrated below skip the generic
    // header/footer entirely. Remaining stages still use the legacy
    // generic header + footer so the workflow stays usable until each
    // story replaces them.
    const bool ownsChrome = (stage == WorkflowStage::Alignment ||
                             stage == WorkflowStage::PointReference);

    if (!ownsChrome) {
        ImGui::Text("Stage %d / 4", static_cast<int>(stage) + 1);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.85f, 1.0f, 1.0f));
        ImGui::TextUnformatted(StageName(stage));
        ImGui::PopStyleColor();
        ImGui::TextWrapped("%s", StageHint(stage));
        ImGui::Separator();
    }

    // Per-stage body
    switch (stage) {
        case WorkflowStage::Alignment:      RenderAlignment(*project, out); break;
        case WorkflowStage::PointReference: RenderPointReference(*project, out); break;
        case WorkflowStage::Morph:          RenderMorph(*project, morphProcessing, morphProgress, out); break;
        case WorkflowStage::TouchUp:        RenderTouchUp(*project, out); break;
    }

    if (!ownsChrome) {
        // Legacy generic stage navigation footer.
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
    }

    ImGui::End();
    return out;
}

// ---------------------------------------------------------------------------
// Stage 1 — Alignment
// ---------------------------------------------------------------------------

void StageSidebar::RenderAlignment(Project& project, Actions& out) {
    namespace I = MvInspector;

    auto& morph  = project.GetMorphMesh();
    auto& target = project.GetTargetMesh();

    // ---- Header ---------------------------------------------------------
    I::DrawStageHeader("STAGE 01 / 04",
                       "Align", "ment",
                       "Position the target mesh to roughly align with the "
                       "morph mesh. Drag the X / Y / Z fields below or grab "
                       "the gizmo handles in the viewport.");

    // ---- Scrollable body ------------------------------------------------
    // Reserve room for a 56 px footer (hairline + 30 px button row + pad)
    // so the Continue button stays pinned to the bottom of the sidebar.
    constexpr float FooterReserveH = 60.0f;
    if (ImGui::BeginChild("##stage1body",
                          ImVec2(0.0f, -FooterReserveH),
                          false,
                          ImGuiWindowFlags_NoBackground)) {

        // ---- Source mesh ------------------------------------------------
        I::DrawSectionLabel("SOURCE MESH");
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
            ImGui::TextWrapped("The MetaHuman head being wrapped.");
            ImGui::PopStyleColor();
        }
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        const std::string srcName = FileNameOrEmpty(morph.filepath);
        const auto srcSlot = I::DrawFileSlot(
            srcName.c_str(),
            "No source mesh",
            morph.isLoaded,
            /*isPrimary=*/true,
            morph.isLoaded ? "Source mesh loaded" : nullptr);
        if (srcSlot.importClicked) out.importMorphMeshRequested = true;

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        if (ImGui::Button("Use Default MetaHuman Head", ImVec2(-FLT_MIN, 30.0f))) {
            out.useDefaultMorphMeshRequested = true;
        }

        // ---- Target mesh ------------------------------------------------
        I::DrawSectionLabel("TARGET MESH");
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
            ImGui::TextWrapped("The custom head model the source will be morphed to.");
            ImGui::PopStyleColor();
        }
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        const std::string tgtName = FileNameOrEmpty(target.filepath);
        const auto tgtSlot = I::DrawFileSlot(
            tgtName.c_str(),
            "No target mesh",
            target.isLoaded,
            /*isPrimary=*/false,
            target.isLoaded ? "Target mesh loaded" : nullptr);
        if (tgtSlot.importClicked) out.importTargetMeshRequested = true;

        // ---- Target textures --------------------------------------------
        I::DrawSectionLabel("TARGET TEXTURES");
        auto& textures = project.GetTargetTextures();
        if (textures.textures.empty()) {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
            ImGui::TextUnformatted("No textures attached.");
            ImGui::PopStyleColor();
        } else {
            // Plain mono list for now — Story 3.8.x will swap each row in
            // for a thumbnail-bearing card when a preview-cache slot is
            // added to the texture model. Listed one-per-row at body size
            // so wrap is rare even with long filenames.
            MvStyle::ScopedFont f(MvStyle::Font.Mono);
            for (const auto& t : textures.textures) {
                ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
                ImGui::BulletText("%s", t.name.c_str());
                ImGui::PopStyleColor();
            }
        }

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        static const char* kSlotNames[] = {
            "Albedo", "Normal Map", "Roughness", "AO", "Emissive", "Metallic"
        };
        static int sSelectedSlot = 0;
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
        ImGui::Combo("##texSlot", &sSelectedSlot, kSlotNames, IM_ARRAYSIZE(kSlotNames));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("+ Texture", ImVec2(-FLT_MIN, 0.0f))) {
            out.importTextureSlot = kSlotNames[sSelectedSlot];
        }

        // ---- Target transform -------------------------------------------
        I::DrawSectionLabel("TARGET TRANSFORM");

        // Single source of truth for transform values is
        // Project::GetTargetMesh().transform — what the renderer + raycaster
        // read at draw / pick time. The shadow copy in AlignmentData is
        // serializer-only.
        Transform& xform = project.GetTargetMesh().transform;

        // Capture-on-edit-start / emit-on-edit-end pattern: one undo action
        // per drag gesture. The aggregate DragEditResult gives us the
        // start/finish signal regardless of which axis was grabbed.
        auto vec3Differs = [](const Vector3& a, const Vector3& b) {
            return a.x != b.x || a.y != b.y || a.z != b.z;
        };
        auto handleEdit = [&](const I::DragEditResult& r) {
            if (r.editStarted)  transformEditCapture_ = xform;
            if (r.editFinished) {
                // Skip pushing an undo if the gesture was a click without a
                // drag (no value change) — comparing to the captured pre-
                // state filters those out. Vector3 has no operator!= so
                // component-wise compare via the lambda above.
                if (vec3Differs(xform.GetPosition(),     transformEditCapture_.GetPosition())     ||
                    vec3Differs(xform.GetEulerAngles(),  transformEditCapture_.GetEulerAngles())  ||
                    vec3Differs(xform.GetScale(),        transformEditCapture_.GetScale())) {
                    out.transformEditCompleted = true;
                    out.transformEditBefore    = transformEditCapture_;
                    out.transformEditAfter     = xform;
                }
            }
        };

        I::TransformValues3 pos{ xform.GetPosition().x,
                                 xform.GetPosition().y,
                                 xform.GetPosition().z };
        const auto rPos = I::DrawTransformGroup(
            "Position", "cm", pos, /*uniform=*/false, /*dragSpeed=*/0.01f);
        if (rPos.changed) xform.SetPosition(Vector3(pos.x, pos.y, pos.z));
        handleEdit(rPos);

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        const Vector3 eulerNow = xform.GetEulerAngles();
        I::TransformValues3 rot{ eulerNow.x, eulerNow.y, eulerNow.z };
        const auto rRot = I::DrawTransformGroup(
            "Rotation", "\xC2\xB0", rot, /*uniform=*/false, /*dragSpeed=*/0.5f);
        if (rRot.changed) xform.SetRotationFromEuler(rot.x, rot.y, rot.z);
        handleEdit(rRot);

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        // Scale uses the X axis as the source of truth in uniform mode;
        // DrawTransformGroup cascades X→Y/Z internally when uniform=true.
        I::TransformValues3 scl{ xform.GetScale().x,
                                 xform.GetScale().y,
                                 xform.GetScale().z };
        const auto rScl = I::DrawTransformGroup(
            "Scale", "\xC3\x97", scl, /*uniform=*/true, /*dragSpeed=*/0.01f);
        if (rScl.changed) xform.SetScale(Vector3(scl.x, scl.y, scl.z));
        handleEdit(rScl);

        ImGui::Dummy(ImVec2(0.0f, 8.0f));

        // Reset / Auto-align button row — 2 equal-width ghost buttons.
        if (ImGui::BeginTable("##xformActions", 2,
                              ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_NoPadOuterX)) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            if (ImGui::Button("Reset", ImVec2(-FLT_MIN, 30.0f))) {
                out.resetTransformRequested = true;
            }

            ImGui::TableNextColumn();
            // Auto-align is a stub for now — the real correspondence-aware
            // alignment ships with the Wrap Engine in Sprint 4.
            ImGui::BeginDisabled();
            ImGui::Button("Auto-align", ImVec2(-FLT_MIN, 30.0f));
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Coming in a future update");
            }
            ImGui::EndTable();
        }

        // ---- Validation -------------------------------------------------
        I::DrawSectionLabel("VALIDATION");
        {
            const std::string srcVal = morph.isLoaded && morph.mesh
                ? std::to_string(static_cast<int>(morph.mesh->GetVertices().size())) + " verts"
                : "—";
            I::DrawValidationRow("Source loaded", srcVal.c_str(),
                                 morph.isLoaded ? I::ValidationStatus::Ok
                                                : I::ValidationStatus::Error);
        }
        {
            const std::string tgtVal = target.isLoaded && target.mesh
                ? std::to_string(static_cast<int>(target.mesh->GetVertices().size())) + " verts"
                : "—";
            I::DrawValidationRow("Target loaded", tgtVal.c_str(),
                                 target.isLoaded ? I::ValidationStatus::Ok
                                                 : I::ValidationStatus::Error);
        }

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
    }
    ImGui::EndChild();

    // ---- Footer ---------------------------------------------------------
    // Stage 1 has no Previous (it's the first stage); Continue enabled when
    // both meshes are loaded, matching Project::CanProceedToNextStage.
    const bool canContinue = morph.isLoaded && target.isLoaded;
    const auto footer = I::DrawInspectorFooter(
        "STEP 01 OF 04", /*previousEnabled=*/false, /*continueEnabled=*/canContinue,
        "Continue", /*finishStyle=*/false);
    if (footer.continueClicked) out.nextStageRequested = true;
    if (footer.previousClicked) out.previousStageRequested = true;
}

// ---------------------------------------------------------------------------
// Stage 2 — Point Reference
// ---------------------------------------------------------------------------

void StageSidebar::RenderPointReference(Project& project, Actions& out) {
    namespace I = MvInspector;
    auto& data = project.GetPointReferenceData();

    // Pre-tally correspondence counts — feeds the stat cards and the
    // Continue gating logic below.
    int paired = 0;
    int srcOnly = 0;
    int tgtOnly = 0;
    for (const auto& c : data.correspondences) {
        const bool hasM = c.morphMeshVertexIndex >= 0;
        const bool hasT = c.targetMeshVertexIndex >= 0;
        if (hasM && hasT) ++paired;
        else if (hasM)    ++srcOnly;
        else if (hasT)    ++tgtOnly;
    }

    // ---- Header ---------------------------------------------------------
    I::DrawStageHeader("STAGE 02 / 04",
                       "Point ", "reference",
                       "Click each mesh at matching anatomical landmarks. "
                       "Pair every placed point on one mesh with its match "
                       "on the other before continuing.");

    // ---- Scrollable body ------------------------------------------------
    constexpr float FooterReserveH = 60.0f;
    if (ImGui::BeginChild("##stage2body",
                          ImVec2(0.0f, -FooterReserveH),
                          false,
                          ImGuiWindowFlags_NoBackground)) {

        // ---- Stat cards row -----------------------------------------------
        // 3 equal-width cards: PAIRED (blue) / SOURCE ONLY (amber) /
        // TARGET ONLY (amber when >0, success green when 0). Drawn manually
        // via ImDrawList so we own the centre alignment + accent colours.
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            const float availW  = ImGui::GetContentRegionAvail().x;
            constexpr float Gap = 6.0f;
            constexpr float CardH = 56.0f;
            const float cardW = (availW - Gap * 2.0f) / 3.0f;

            struct Card {
                const char* label;
                int value;
                ImVec4 accent;
            };
            const Card cards[3] = {
                { "PAIRED",      paired,
                  paired > 0 ? MvStyle::MvBlue500 : MvStyle::MvFg4 },
                { "SOURCE ONLY", srcOnly,
                  srcOnly > 0 ? MvStyle::MvWarning : MvStyle::MvFg4 },
                { "TARGET ONLY", tgtOnly,
                  tgtOnly > 0 ? MvStyle::MvWarning : MvStyle::MvSuccess },
            };

            for (int i = 0; i < 3; ++i) {
                const ImVec2 cmin(origin.x + i * (cardW + Gap), origin.y);
                const ImVec2 cmax(cmin.x + cardW, cmin.y + CardH);
                dl->AddRectFilled(cmin, cmax, ImGui::ColorConvertFloat4ToU32(MvStyle::MvBg1), 6.0f);
                dl->AddRect      (cmin, cmax, ImGui::ColorConvertFloat4ToU32(MvStyle::MvLine2), 6.0f, 0, 1.0f);

                // Big count number — Heading, accent colour, top half.
                {
                    MvStyle::ScopedFont f(MvStyle::Font.Heading);
                    char buf[16]; std::snprintf(buf, sizeof(buf), "%d", cards[i].value);
                    const ImVec2 ts = ImGui::CalcTextSize(buf);
                    dl->AddText(ImVec2(cmin.x + (cardW - ts.x) * 0.5f, cmin.y + 6.0f),
                                ImGui::ColorConvertFloat4ToU32(cards[i].accent), buf);
                }
                // Mono uppercase label — bottom half, MvFg3.
                {
                    MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
                    const ImVec2 ts = ImGui::CalcTextSize(cards[i].label);
                    dl->AddText(ImVec2(cmin.x + (cardW - ts.x) * 0.5f, cmax.y - ts.y - 8.0f),
                                ImGui::ColorConvertFloat4ToU32(MvStyle::MvFg3), cards[i].label);
                }
            }
            ImGui::Dummy(ImVec2(availW, CardH));
        }

        // ---- Points list ------------------------------------------------
        // Scrollable list of every correspondence with two presence squares
        // on the right (S = source/morph mesh, T = target mesh). Filled
        // square = a vertex is set on that side, hollow square = orphan.
        // Click a row to select the point in both viewports — the row
        // matching `selectedPointIndexExternal_` (mirrored from the primary
        // viewport at Render entry) renders highlighted.
        I::DrawSectionLabel("POINTS");
        if (data.correspondences.empty()) {
            MvStyle::ScopedFont fE(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
            ImGui::TextUnformatted("No points placed yet. Click the source "
                                   "or target mesh to begin.");
            ImGui::PopStyleColor();
        } else {
            // Cap the list height so the rest of the sidebar (settings +
            // Clear All button) stays reachable without scrolling the whole
            // panel — 8 rows × 26 px + a little chrome covers the typical
            // usage; scroll for more.
            constexpr float RowH      = 26.0f;
            constexpr float MaxListH  = 8.0f * RowH + 8.0f;
            const float listH = std::min(
                MaxListH,
                static_cast<float>(data.correspondences.size()) * RowH + 4.0f);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, MvStyle::MvBg1);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            if (ImGui::BeginChild("##ptList",
                                  ImVec2(-FLT_MIN, listH),
                                  true,
                                  ImGuiWindowFlags_NoScrollWithMouse)) {

                ImDrawList* dl = ImGui::GetWindowDrawList();
                const float rowW = ImGui::GetContentRegionAvail().x;
                const ImU32 colSrcAccent = ImGui::ColorConvertFloat4ToU32(MvStyle::MvWarning);
                const ImU32 colTgtAccent = ImGui::ColorConvertFloat4ToU32(MvStyle::MvBlue400);
                const ImU32 colMissing   = ImGui::ColorConvertFloat4ToU32(MvStyle::MvFg4);

                for (size_t i = 0; i < data.correspondences.size(); ++i) {
                    const auto& corr = data.correspondences[i];
                    const bool hasSrc = corr.morphMeshVertexIndex  >= 0;
                    const bool hasTgt = corr.targetMeshVertexIndex >= 0;
                    const bool isSelected =
                        (static_cast<int>(i) == selectedPointIndexExternal_);

                    ImGui::PushID(static_cast<int>(i));
                    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
                    if (ImGui::InvisibleButton("##row", ImVec2(rowW, RowH))) {
                        out.selectPointIndex = static_cast<int>(i);
                    }
                    const bool hovered = ImGui::IsItemHovered();
                    ImGui::PopID();
                    const ImVec2 rowMax(rowMin.x + rowW, rowMin.y + RowH);

                    // Row background — selected wins over hover.
                    if (isSelected) {
                        ImVec4 bg = MvStyle::MvBlue500; bg.w = 0.18f;
                        dl->AddRectFilled(rowMin, rowMax,
                                          ImGui::ColorConvertFloat4ToU32(bg));
                    } else if (hovered) {
                        ImVec4 bg = MvStyle::MvBlue500; bg.w = 0.06f;
                        dl->AddRectFilled(rowMin, rowMax,
                                          ImGui::ColorConvertFloat4ToU32(bg));
                    }

                    // Point ID — mono, MvFg1 when selected, MvFg2 otherwise.
                    {
                        char idBuf[16];
                        std::snprintf(idBuf, sizeof(idBuf), "%02d", corr.pointID);
                        MvStyle::ScopedFont f(MvStyle::Font.Mono);
                        const ImVec2 ts = ImGui::CalcTextSize(idBuf);
                        const float fs = ImGui::GetFontSize();
                        const ImU32 col = ImGui::ColorConvertFloat4ToU32(
                            isSelected ? MvStyle::MvFg1 : MvStyle::MvFg2);
                        dl->AddText(ImVec2(rowMin.x + 12.0f,
                                           rowMin.y + (RowH - fs) * 0.5f),
                                    col, idBuf);
                        (void)ts;
                    }

                    // Two presence squares on the right (S then T). Filled
                    // = vertex set on that side, hollow = orphan. The
                    // empty-list block above also covers the "no points
                    // yet" case.
                    constexpr float SqSize = 10.0f;
                    constexpr float SqPad  = 6.0f;
                    constexpr float RightPad = 12.0f;
                    const float yMid = rowMin.y + RowH * 0.5f;
                    const float tgtRight = rowMax.x - RightPad;
                    const float tgtLeft  = tgtRight - SqSize;
                    const float srcRight = tgtLeft - SqPad;
                    const float srcLeft  = srcRight - SqSize;

                    auto drawSquare = [&](float x0, float x1,
                                          bool filled, ImU32 accentCol) {
                        const ImVec2 amin(x0, yMid - SqSize * 0.5f);
                        const ImVec2 amax(x1, yMid + SqSize * 0.5f);
                        if (filled) {
                            dl->AddRectFilled(amin, amax, accentCol, 2.0f);
                        } else {
                            dl->AddRect(amin, amax, colMissing, 2.0f, 0, 1.0f);
                        }
                    };
                    drawSquare(srcLeft, srcRight, hasSrc, colSrcAccent);
                    drawSquare(tgtLeft, tgtRight, hasTgt, colTgtAccent);

                    if (hovered) {
                        ImGui::SetTooltip(
                            "Point %d \xC2\xB7 source %s \xC2\xB7 target %s",
                            corr.pointID,
                            hasSrc ? "set" : "missing",
                            hasTgt ? "set" : "missing");
                    }
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        // ---- Placement tools (stub buttons per design) --------------------
        I::DrawSectionLabel("PLACEMENT TOOLS");
        if (ImGui::BeginTable("##placeTools", 2,
                              ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_NoPadOuterX)) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::BeginDisabled();
            ImGui::Button("Auto-detect", ImVec2(-FLT_MIN, 30.0f));
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Coming in a future update");
            }

            ImGui::TableNextColumn();
            ImGui::BeginDisabled();
            ImGui::Button("Load preset", ImVec2(-FLT_MIN, 30.0f));
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Coming in a future update");
            }

            ImGui::EndTable();
        }

        // ---- Settings (existing controls, restyled) -----------------------
        I::DrawSectionLabel("SETTINGS");

        bool symmetry = data.symmetryEnabled;
        if (ImGui::Checkbox("Mirror across symmetry axis", &symmetry)) {
            data.symmetryEnabled = symmetry;
        }

        int axisIdx = AxisToIndex(data.symmetryAxis);
        const char* axes[] = { "X axis", "Y axis", "Z axis" };
        ImGui::BeginDisabled(!symmetry);
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::Combo("##symAxis", &axisIdx, axes, 3)) {
            data.symmetryAxis = IndexToAxis(axisIdx);
        }
        ImGui::PopItemWidth();
        ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##ptSize", &data.pointRenderSize, 4.0f, 32.0f,
                           "Point size %.1f");
        ImGui::PopItemWidth();

        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        if (ImGui::Button("Clear All Points", ImVec2(-FLT_MIN, 30.0f))) {
            out.clearAllPointsRequested = true;
        }

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
    }
    ImGui::EndChild();

    // ---- Footer ---------------------------------------------------------
    // Continue gates on "all points are paired" — paired > 0 and no orphans
    // on either side. Stricter than Project::CanProceedToNextStage's existing
    // count-equality check (which would let a 3-and-3 mismatched layout
    // through). Matches the design's intent: progress only when every
    // landmark has its counterpart.
    const bool canContinue = paired > 0 && srcOnly == 0 && tgtOnly == 0;
    const auto footer = I::DrawInspectorFooter(
        "STEP 02 OF 04", /*previousEnabled=*/true, /*continueEnabled=*/canContinue,
        "Continue", /*finishStyle=*/false);
    if (footer.continueClicked) out.nextStageRequested = true;
    if (footer.previousClicked) out.previousStageRequested = true;
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

        // Drive directly off morph.previewMode rather than a static — the
        // value is also restored from disk on project load, and a static
        // would keep showing whatever the user last picked in this session.
        int previewIdx = static_cast<int>(morph.previewMode);
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
