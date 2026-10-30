#include "ui/StageSidebar.h"
#include "core/Mesh.h"
#include "core/Project.h"
#include "core/TextureData.h"
#include "core/WrapPreset.h"
#include "ui/MvIcons.h"
#include "ui/MvInspectorPrimitives.h"
#include "ui/MvStyle.h"
#include "utils/Config.h"

#include <imgui.h>

#include <cstdio>
#include <cstring>

#include <algorithm>
#include <array>
#include <filesystem>

namespace MetaVisage {

namespace {

// Story 5.2 (Outcome B) — landmark-weight / final-stiffness ratio above
// which the Stage 3 inspector renders the "cottage cheese" warning row
// (see RenderMorph). Initial threshold per the sprint plan; should be
// refined empirically once the diagnostic dump
// (METAVISAGE_DUMP_RESIDUALS=1) yields real residual data from
// reproduction cases. Hoisted from the function body per PR #24 review
// so the follow-up root-cause spike can find it via grep without
// reading through RenderMorph's body.
constexpr float kLandmarkRatioWarnThreshold = 0.5f;

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
                                           const std::vector<WrapPreset>& userPresets,
                                           int selectedPointIndex) {
    Actions out;
    selectedPointIndexExternal_ = selectedPointIndex;

    // NoMove + NoCollapse + NoTitleBar work together with the dock node flags
    // set by AppWindow (NoUndocking / NoDockingOverMe / HiddenTabBar) to keep
    // the sidebar statically pinned to the right of the app shell.
    //
    // NoScrollbar / NoScrollWithMouse: the per-stage panels each open
    // their own BeginChild scroll region for the body (between the
    // header and the footer). Letting the outer "Stage" window scroll
    // too produced a redundant scrollbar — and one that scrolled the
    // header / footer out of the visible area, which the design pins
    // top + bottom respectively.
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
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
                             stage == WorkflowStage::PointReference ||
                             stage == WorkflowStage::Morph     ||
                             stage == WorkflowStage::TouchUp);

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
        case WorkflowStage::Morph:          RenderMorph(*project, morphProcessing, morphProgress, userPresets, out); break;
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
                       "morph mesh. Drag or double-click the X / Y / Z fields "
                       "below, or grab the gizmo handles in the viewport.");

    // ---- Scrollable body ------------------------------------------------
    // Reserve room for a 56 px footer (hairline + 30 px button row + pad)
    // so the Continue button stays pinned to the bottom of the sidebar.
    constexpr float FooterReserveH = 60.0f;
    if (ImGui::BeginChild("##stage1body",
                          ImVec2(0.0f, -FooterReserveH),
                          false,
                          ImGuiWindowFlags_NoBackground)) {

        I::DrawSectionLabel("SOURCE MESH");

        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
            ImGui::TextWrapped("The MetaHuman head being wrapped.");
            ImGui::PopStyleColor();
        }
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        const std::string srcName = FileNameOrEmpty(morph.filepath);
        {
            const auto srcSlot = I::DrawFileSlot(
                srcName.c_str(),
                "No source mesh",
                morph.isLoaded,
                /*isPrimary=*/true,
                morph.isLoaded ? "Source mesh loaded" : nullptr);
            if (srcSlot.importClicked) out.importMorphMeshRequested = true;
        }

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        if (ImGui::Button("Use Default MetaHuman Head", ImVec2(-FLT_MIN, 30.0f))) {
            out.useDefaultMorphMeshRequested = true;
        }

        // ---- Target mesh ------------------------------------------------
        // Target mesh is the head the source will be morphed to.
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
        {
            I::DrawSectionLabel("TARGET TEXTURES");
            auto& textures = project.GetTargetTextures();
            if (textures.textures.empty()) {
                MvStyle::ScopedFont f(MvStyle::Font.BodySm);
                ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
                ImGui::TextUnformatted("No textures attached.");
                ImGui::PopStyleColor();
            } else {
                // Plain mono list for now -- Story 3.8.x will swap each row
                // in for a thumbnail-bearing card when a preview-cache slot
                // is added to the texture model. Listed one-per-row at body
                // size so wrap is rare even with long filenames.
                MvStyle::ScopedFont f(MvStyle::Font.Mono);
                for (const auto& t : textures.textures) {
                    ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
                    ImGui::BulletText("%s", t.name.c_str());
                    ImGui::PopStyleColor();
                }
            }

            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            static const char* kSlotNamesAll[] = {
                "Albedo", "Normal Map", "Roughness", "AO", "Emissive", "Metallic"
            };
            const char* const* kSlotNames = kSlotNamesAll;
            const int kSlotCount = IM_ARRAYSIZE(kSlotNamesAll);
            static int sSelectedSlot = 0;
            if (sSelectedSlot >= kSlotCount) sSelectedSlot = 0;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            ImGui::Combo("##texSlot", &sSelectedSlot, kSlotNames, kSlotCount);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("+ Texture", ImVec2(-FLT_MIN, 0.0f))) {
                out.importTextureSlot = kSlotNames[sSelectedSlot];
            }

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

        // Sprint 9.3 — scale position drag speed to the target's size so a
        // cm-scale head moves usefully per pixel (0.1% of the bounds
        // diagonal; the fixed 0.01 crawled). Typed entry covers precision.
        float posDragSpeed = 0.01f;
        if (target.isLoaded && target.mesh) {
            posDragSpeed = std::max(posDragSpeed,
                                    target.mesh->GetBounds().Size().Length() * 0.001f);
        }
        I::TransformValues3 pos{ xform.GetPosition().x,
                                 xform.GetPosition().y,
                                 xform.GetPosition().z };
        const auto rPos = I::DrawTransformGroup(
            "Position", "cm", pos, /*uniform=*/false, posDragSpeed);
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
        // Scrollable list of every correspondence. Each row matches the
        // design's stage2.jsx layout:
        //
        //   [bd] Point N         [S][T] [×]
        //        STATUS LABEL
        //
        // - bd: 32 px badge, transparent fill + 1 px border in the point's
        //       palette colour, mono "0N" centred inside.
        // - "Point N":   Body text, MvFg1.
        // - STATUS:      MonoSm; "PAIRED" green when both sides set,
        //                "NEEDS A" / "NEEDS B" amber when one side missing.
        // - [S][T]:      two 12 px squares — filled in palette colour when
        //                that side has a vertex, hollow MvLine2 otherwise.
        //                Source first, target second to match the design.
        // - [×]:         deletes the row's correspondence (forwards to
        //                AppWindow's existing delete path with this index).
        //
        // Click anywhere on the row body to select; click the X to delete.
        I::DrawSectionLabel("POINTS");
        if (data.correspondences.empty()) {
            MvStyle::ScopedFont fE(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
            ImGui::TextUnformatted("No points placed yet. Click the source "
                                   "or target mesh to begin.");
            ImGui::PopStyleColor();
        } else {
            // Per-point palette — mirrors stage2.jsx pointColor(idx). Cycles
            // by pointID so colours stay stable across deletions.
            static const ImVec4 kPalette[] = {
                ImVec4(0x3D/255.f, 0xA5/255.f, 0xF5/255.f, 1.f), // blue 500
                ImVec4(0x7B/255.f, 0xC2/255.f, 0xFA/255.f, 1.f), // blue 300
                ImVec4(0x3F/255.f, 0xD6/255.f, 0x9E/255.f, 1.f), // success
                ImVec4(0xF2/255.f, 0xB5/255.f, 0x47/255.f, 1.f), // warning
                ImVec4(0xF2/255.f, 0x6A/255.f, 0x6A/255.f, 1.f), // danger
                ImVec4(0xC5/255.f, 0x8C/255.f, 0xF7/255.f, 1.f), // violet
                ImVec4(0x5D/255.f, 0xCA/255.f, 0xE0/255.f, 1.f), // cyan
            };
            constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

            // 60 px-tall rows: 32 badge + 14 + 14 lines. Cap visible height
            // at ~6 rows so the SETTINGS block below stays reachable.
            constexpr float RowH     = 60.0f;
            constexpr float MaxListH = 6.0f * RowH + 8.0f;
            const float listH = std::min(
                MaxListH,
                static_cast<float>(data.correspondences.size()) * RowH + 4.0f);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, MvStyle::MvBg1);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            // No NoScrollWithMouse — the user must be able to wheel the list
            // when more than ~6 points exist. Border stays for the framed
            // look.
            if (ImGui::BeginChild("##ptList",
                                  ImVec2(-FLT_MIN, listH),
                                  true,
                                  ImGuiWindowFlags_None)) {

                ImDrawList* dl = ImGui::GetWindowDrawList();
                const float rowW = ImGui::GetContentRegionAvail().x;
                const ImU32 colMissingBorder =
                    ImGui::ColorConvertFloat4ToU32(MvStyle::MvLine2);

                for (size_t i = 0; i < data.correspondences.size(); ++i) {
                    const auto& corr = data.correspondences[i];
                    const bool hasSrc = corr.morphMeshVertexIndex  >= 0;
                    const bool hasTgt = corr.targetMeshVertexIndex >= 0;
                    const bool isSelected =
                        (static_cast<int>(i) == selectedPointIndexExternal_);

                    const ImVec4 accent =
                        kPalette[((corr.pointID >= 0 ? corr.pointID : 0) % kPaletteSize)];
                    const ImU32 accentU = ImGui::ColorConvertFloat4ToU32(accent);

                    ImGui::PushID(static_cast<int>(i));

                    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
                    constexpr float DelBtnW = 28.0f;
                    const float bodyW = rowW - DelBtnW;

                    // Row body button — owns row-select hit-testing.
                    if (ImGui::InvisibleButton("##sel", ImVec2(bodyW, RowH))) {
                        out.selectPointIndex = static_cast<int>(i);
                    }
                    const bool rowHovered = ImGui::IsItemHovered();
                    const ImVec2 bodyMax(rowMin.x + bodyW, rowMin.y + RowH);

                    // X delete button — second item, adjacent on the same row.
                    ImGui::SameLine(0.0f, 0.0f);
                    const ImVec2 delMin = ImGui::GetCursorScreenPos();
                    if (ImGui::InvisibleButton("##del", ImVec2(DelBtnW, RowH))) {
                        out.deletePointIndex = static_cast<int>(i);
                    }
                    const bool delHovered = ImGui::IsItemHovered();
                    const ImVec2 delMax(delMin.x + DelBtnW, delMin.y + RowH);

                    const ImVec2 rowMax(rowMin.x + rowW, rowMin.y + RowH);

                    // Row background — selected wins over hover.
                    if (isSelected) {
                        ImVec4 bg = MvStyle::MvBlue500; bg.w = 0.18f;
                        dl->AddRectFilled(rowMin, rowMax,
                                          ImGui::ColorConvertFloat4ToU32(bg));
                    } else if (rowHovered || delHovered) {
                        ImVec4 bg = MvStyle::MvBlue500; bg.w = 0.06f;
                        dl->AddRectFilled(rowMin, rowMax,
                                          ImGui::ColorConvertFloat4ToU32(bg));
                    }

                    // ---- Badge (32×32, point colour outline) ------------
                    constexpr float BadgeSize = 32.0f;
                    constexpr float BadgePadL = 10.0f;
                    const ImVec2 bMin(rowMin.x + BadgePadL,
                                      rowMin.y + (RowH - BadgeSize) * 0.5f);
                    const ImVec2 bMax(bMin.x + BadgeSize, bMin.y + BadgeSize);
                    // Faint tinted fill + 1 px coloured border.
                    ImVec4 badgeFill = accent; badgeFill.w = 0.10f;
                    dl->AddRectFilled(bMin, bMax,
                                      ImGui::ColorConvertFloat4ToU32(badgeFill),
                                      4.0f);
                    dl->AddRect(bMin, bMax, accentU, 4.0f, 0, 1.0f);
                    {
                        char idBuf[8];
                        std::snprintf(idBuf, sizeof(idBuf), "%02d", corr.pointID);
                        MvStyle::ScopedFont f(MvStyle::Font.Mono);
                        const ImVec2 ts = ImGui::CalcTextSize(idBuf);
                        const float fs = ImGui::GetFontSize();
                        dl->AddText(ImVec2(bMin.x + (BadgeSize - ts.x) * 0.5f,
                                           bMin.y + (BadgeSize - fs) * 0.5f),
                                    accentU, idBuf);
                    }

                    // ---- Text stack: "Point N" + status -----------------
                    const float textX = bMax.x + 12.0f;
                    {
                        // Body label
                        char nameBuf[32];
                        std::snprintf(nameBuf, sizeof(nameBuf), "Point %d", corr.pointID);
                        MvStyle::ScopedFont f(MvStyle::Font.BodyMd);
                        const float fs = ImGui::GetFontSize();
                        const float yTop = rowMin.y + (RowH * 0.5f) - fs - 1.0f;
                        dl->AddText(ImVec2(textX, yTop),
                                    ImGui::ColorConvertFloat4ToU32(MvStyle::MvFg1),
                                    nameBuf);
                    }
                    {
                        // Status sub-label.
                        const char* statusText;
                        ImVec4 statusColor;
                        if (hasSrc && hasTgt) {
                            statusText = "PAIRED";
                            statusColor = MvStyle::MvSuccess;
                        } else if (hasSrc && !hasTgt) {
                            statusText = "NEEDS B";
                            statusColor = MvStyle::MvWarning;
                        } else if (!hasSrc && hasTgt) {
                            statusText = "NEEDS A";
                            statusColor = MvStyle::MvWarning;
                        } else {
                            statusText = "EMPTY";
                            statusColor = MvStyle::MvFg4;
                        }
                        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
                        const float yBot = rowMin.y + (RowH * 0.5f) + 3.0f;
                        dl->AddText(ImVec2(textX, yBot),
                                    ImGui::ColorConvertFloat4ToU32(statusColor),
                                    statusText);
                    }

                    // ---- Right cluster: two squares + X icon ------------
                    constexpr float SqSize  = 12.0f;
                    constexpr float SqPad   = 4.0f;
                    constexpr float SqClusterPad = 8.0f; // gap between squares and X
                    const float yMid = rowMin.y + RowH * 0.5f;

                    // X icon (Lucide MV_ICON_X) is centred inside the
                    // delete-button hit area on the right edge.
                    const ImU32 xCol = ImGui::ColorConvertFloat4ToU32(
                        delHovered ? MvStyle::MvDanger : MvStyle::MvFg3);
                    if (MvStyle::Font.IconSm) {
                        ImFont* iconFont = MvStyle::Font.IconSm;
                        constexpr float xSize = 14.0f;
                        const ImVec2 ts = iconFont->CalcTextSizeA(
                            xSize, FLT_MAX, 0.0f, MV_ICON_X);
                        dl->AddText(iconFont, xSize,
                                    ImVec2((delMin.x + delMax.x - ts.x) * 0.5f,
                                           yMid - ts.y * 0.5f),
                                    xCol, MV_ICON_X);
                    }

                    // Squares — Source then Target. Both filled in the
                    // point's accent colour when present; hollow MvLine2
                    // outline when missing. Placed just left of the X.
                    const float xClusterRight = delMin.x - SqClusterPad;
                    const float tgtRight = xClusterRight;
                    const float tgtLeft  = tgtRight - SqSize;
                    const float srcRight = tgtLeft - SqPad;
                    const float srcLeft  = srcRight - SqSize;

                    auto drawSquare = [&](float x0, float x1, bool filled) {
                        const ImVec2 amin(x0, yMid - SqSize * 0.5f);
                        const ImVec2 amax(x1, yMid + SqSize * 0.5f);
                        if (filled) {
                            dl->AddRectFilled(amin, amax, accentU, 2.0f);
                        } else {
                            dl->AddRect(amin, amax, colMissingBorder,
                                        2.0f, 0, 1.0f);
                        }
                    };
                    drawSquare(srcLeft, srcRight, hasSrc);
                    drawSquare(tgtLeft, tgtRight, hasTgt);

                    if (rowHovered || delHovered) {
                        if (delHovered) {
                            ImGui::SetTooltip("Delete point %d", corr.pointID);
                        } else {
                            ImGui::SetTooltip(
                                "Point %d \xC2\xB7 source %s \xC2\xB7 target %s",
                                corr.pointID,
                                hasSrc ? "set" : "missing",
                                hasTgt ? "set" : "missing");
                        }
                    }

                    ImGui::PopID();
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

namespace {

// Truncate `text` so its rendered width fits within `maxWidth` using the
// currently pushed ImGui font. Appends a horizontal-ellipsis glyph (U+2026)
// when truncation occurs. UTF-8-aware: walks codepoints, never splits mid-
// sequence — so user preset names like "Casey's Café" don't blow up.
//
// PRECONDITION: caller MUST push the font they want measured against
// (MvStyle::ScopedFont or ImGui::PushFont) before calling. The function
// uses ImGui::CalcTextSize which reads the active font from ImGuiContext —
// pass a font as a parameter and you'll get sized-against-the-default-font
// truncation, which is almost certainly not what the call site wants.
//
// Today's call sites (DrawPresetCard name + sub-label) both wrap the call
// in a ScopedFont block. Add a similar wrap before any new call.
std::string TruncateToWidth(const char* text, float maxWidth) {
    if (!text || !text[0]) return {};
    if (ImGui::CalcTextSize(text).x <= maxWidth) return text;

    constexpr const char* kEllipsis = "\xE2\x80\xA6"; // U+2026
    const float ellipsisW = ImGui::CalcTextSize(kEllipsis).x;
    if (maxWidth <= ellipsisW) return kEllipsis;

    const float budget = maxWidth - ellipsisW;
    std::string out;
    out.reserve(std::strlen(text) + 4);

    // Append codepoints in place; on overshoot, roll back the last codepoint
    // and stop. O(n) in the input length — no per-iteration allocation.
    const char* cur = text;
    while (*cur) {
        const unsigned char b = static_cast<unsigned char>(*cur);
        size_t bytes = 1;
        if      ((b & 0xE0) == 0xC0) bytes = 2;
        else if ((b & 0xF0) == 0xE0) bytes = 3;
        else if ((b & 0xF8) == 0xF0) bytes = 4;

        const size_t before = out.size();
        out.append(cur, bytes);
        if (ImGui::CalcTextSize(out.c_str()).x > budget) {
            out.resize(before); // roll back the codepoint that overshot
            break;
        }
        cur += bytes;
    }
    out += kEllipsis;
    return out;
}

ImU32 UiCol(const ImVec4& c) {
    return ImGui::ColorConvertFloat4ToU32(c);
}

// Returns true on click. Caller decides whether to apply the preset; the
// helper just paints the card and dispatches the hit-test.
//
// `thumbnail` is optional (Story 5.3): when non-zero, an RGBA texture is
// rendered in place of the Lucide glyph, scaled to a 28 px square (matches
// the visual weight of the existing 22 px glyph + 4 px breathing room
// before the name row). Used by the Stage 4 brush card grid to surface
// brush-effect preview PNGs from `assets/brushes/`. Stage 3 preset cards
// leave it 0 and keep their glyphs.
bool DrawPresetCard(const char* idStr,
                    const char* name,
                    const char* sub,
                    const char* glyph,
                    bool active,
                    bool disabled,
                    ImTextureID thumbnail = 0) {
    constexpr float CardH = 76.0f;
    ImGui::PushID(idStr);
    const ImVec2 pmin = ImGui::GetCursorScreenPos();
    const float availW = ImGui::GetContentRegionAvail().x;
    const ImVec2 pmax(pmin.x + availW, pmin.y + CardH);

    ImGui::BeginDisabled(disabled);
    const bool clicked = ImGui::InvisibleButton("##card", ImVec2(availW, CardH));
    ImGui::EndDisabled();
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background + border. Active wins over hover.
    if (active) {
        ImVec4 fill = MvStyle::MvBlue500; fill.w = 0.08f;
        dl->AddRectFilled(pmin, pmax, ImGui::ColorConvertFloat4ToU32(fill), 6.0f);
        dl->AddRect(pmin, pmax,
                    ImGui::ColorConvertFloat4ToU32(MvStyle::MvBlue500),
                    6.0f, 0, 1.5f);
    } else {
        const ImU32 bgCol = hovered && !disabled
            ? ImGui::ColorConvertFloat4ToU32(MvStyle::MvBg2)
            : ImGui::ColorConvertFloat4ToU32(MvStyle::MvBg1);
        dl->AddRectFilled(pmin, pmax, bgCol, 6.0f);
        const ImU32 borderCol = disabled
            ? ImGui::ColorConvertFloat4ToU32(MvStyle::MvLine3)
            : ImGui::ColorConvertFloat4ToU32(MvStyle::MvLine2);
        dl->AddRect(pmin, pmax, borderCol, 6.0f, 0, 1.0f);
    }

    // Centred icon area (y=8..32). Prefer the PNG thumbnail when a caller
    // passes one (Stage 4 brush previews); otherwise fall back to the
    // Lucide glyph at the same vertical slot. Disabled state mutes the
    // image with a 50 % alpha overlay, mirroring the glyph's MvFg4
    // treatment so the two paths feel consistent.
    if (thumbnail) {
        constexpr float kThumbSize = 28.0f;
        const ImVec2 thumbMin(pmin.x + (availW - kThumbSize) * 0.5f,
                              pmin.y + 4.0f);
        const ImVec2 thumbMax(thumbMin.x + kThumbSize,
                              thumbMin.y + kThumbSize);
        const ImU32 tint = disabled
            ? IM_COL32(255, 255, 255, 128)
            : IM_COL32_WHITE;
        dl->AddImage(thumbnail, thumbMin, thumbMax,
                     ImVec2(0, 0), ImVec2(1, 1), tint);
    } else if (glyph && MvStyle::Font.Icon) {
        ImFont* icon = MvStyle::Font.Icon;
        constexpr float iconSize = 22.0f;
        const ImVec2 ts = icon->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, glyph);
        const ImVec4 iconColor =
            disabled ? MvStyle::MvFg4 :
            active   ? MvStyle::MvBlue400 : MvStyle::MvFg2;
        dl->AddText(icon, iconSize,
                    ImVec2(pmin.x + (availW - ts.x) * 0.5f,
                           pmin.y + 8.0f),
                    ImGui::ColorConvertFloat4ToU32(iconColor),
                    glyph);
    }

    // 8 px on each side keeps the truncated text from butting up against
    // the card border. Long preset names (user-provided via the Save modal)
    // would otherwise overflow into adjacent cards — see the Sprint 4 user
    // feedback on "Casey's Custom Preset" and the bundled descriptions.
    constexpr float kTextPad = 8.0f;
    const float maxTextW = availW - 2.0f * kTextPad;

    // Name (Body Md) — centred horizontally, ellipsised on overflow.
    if (name && name[0]) {
        MvStyle::ScopedFont f(MvStyle::Font.BodyMd);
        const std::string drawn = TruncateToWidth(name, maxTextW);
        const ImVec2 ts = ImGui::CalcTextSize(drawn.c_str());
        const ImVec4 nameColor = disabled ? MvStyle::MvFg4 : MvStyle::MvFg1;
        dl->AddText(ImVec2(pmin.x + (availW - ts.x) * 0.5f,
                           pmin.y + 36.0f),
                    ImGui::ColorConvertFloat4ToU32(nameColor),
                    drawn.c_str());
    }

    // Sub-label (MonoSm MvFg3) — same treatment.
    if (sub && sub[0]) {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        const std::string drawn = TruncateToWidth(sub, maxTextW);
        const ImVec2 ts = ImGui::CalcTextSize(drawn.c_str());
        dl->AddText(ImVec2(pmin.x + (availW - ts.x) * 0.5f,
                           pmin.y + CardH - ts.y - 8.0f),
                    ImGui::ColorConvertFloat4ToU32(MvStyle::MvFg3),
                    drawn.c_str());
    }

    if (hovered && disabled) {
        ImGui::SetTooltip("Coming in a future update");
    }

    return clicked && !disabled;
}

bool DrawBrushPaletteTile(const char* idStr,
                          const char* name,
                          const char* glyph,
                          const char* hotkey,
                          bool active,
                          ImTextureID thumbnail = 0) {
    constexpr float TileSize = 52.0f;
    constexpr float Radius = 4.0f;

    ImGui::PushID(idStr);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + TileSize, min.y + TileSize);
    const bool clicked = ImGui::InvisibleButton("##brushTile",
                                                ImVec2(TileSize, TileSize));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (active && thumbnail) {
        dl->AddRectFilled(min, max, UiCol(MvStyle::MvBg2), Radius);
        dl->AddRect(min, max, UiCol(MvStyle::MvBlue500), Radius, 0, 1.5f);
    } else if (active) {
        dl->AddRectFilled(min, max, UiCol(MvStyle::MvBlue500), Radius);
        dl->AddRect(min, max, UiCol(MvStyle::MvBlue400), Radius, 0, 1.0f);
    } else {
        dl->AddRectFilled(min, max, UiCol(hovered ? MvStyle::MvBg2
                                                  : MvStyle::MvBg1), Radius);
        dl->AddRect(min, max, UiCol(MvStyle::MvLine2), Radius, 0, 1.0f);
    }

    if (thumbnail) {
        constexpr float ThumbSize = 34.0f;
        const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
        const ImVec2 imgMin(center.x - ThumbSize * 0.5f,
                            center.y - ThumbSize * 0.5f);
        const ImVec2 imgMax(imgMin.x + ThumbSize, imgMin.y + ThumbSize);
        const ImU32 tint = IM_COL32_WHITE;
        dl->AddImage(thumbnail, imgMin, imgMax,
                     ImVec2(0, 0), ImVec2(1, 1), tint);
    } else if (glyph && MvStyle::Font.Icon) {
        ImFont* icon = MvStyle::Font.Icon;
        constexpr float IconSize = 21.0f;
        const ImVec2 ts = icon->CalcTextSizeA(IconSize, FLT_MAX, 0.0f, glyph);
        const ImVec4 iconColor = active
            ? MvStyle::MvBg0
            : MvStyle::MvFg2;
        dl->AddText(icon, IconSize,
                    ImVec2(min.x + (TileSize - ts.x) * 0.5f,
                           min.y + (TileSize - ts.y) * 0.5f),
                    UiCol(iconColor),
                    glyph);
    }

    if (hotkey && hotkey[0]) {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        const ImVec2 ts = ImGui::CalcTextSize(hotkey);
        const ImVec4 keyColor = active
            ? MvStyle::MvBg0
            : MvStyle::MvFg4;
        dl->AddText(ImVec2(max.x - ts.x - 5.0f,
                           max.y - ts.y - 4.0f),
                    UiCol(keyColor),
                    hotkey);
    }

    if (hovered && name && name[0]) {
        ImGui::SetTooltip("%s", name);
    }

    return clicked;
}
} // namespace

void StageSidebar::RenderMorph(Project& project, bool processing, float progress,
                               const std::vector<WrapPreset>& userPresets,
                               Actions& out) {
    namespace I = MvInspector;
    auto& morph = project.GetMorphData();

    // Auto-derive the active preset card by parameter equality whenever the
    // tracked id is empty — handles project load (no explicit serialization
    // of the active id), the case where the user drags a slider back to a
    // preset's exact value, and the just-after-save case where a new user
    // preset becomes the canonical match for the current params. Built-ins
    // get first dibs so a user preset that happens to duplicate a built-in
    // doesn't shadow it. The param-edit clear path lower in this function
    // clears the id; the next frame this re-checks.
    if (activePresetId_.empty()) {
        activePresetId_ = WrapPreset::FindMatchingBuiltInId(morph);
        if (activePresetId_.empty()) {
            for (const auto& preset : userPresets) {
                if (preset.MatchesParamsOf(morph)) {
                    activePresetId_ = preset.id;
                    break;
                }
            }
        }
    }

    // ---- Header ---------------------------------------------------------
    I::DrawStageHeader("STAGE 03 / 04",
                       "Morph", "ing",
                       "Tune the wrap parameters and run the algorithm. "
                       "Compare Source / Target / Deformed in the viewport "
                       "before accepting.");

    // ---- Scrollable body ------------------------------------------------
    constexpr float FooterReserveH = 60.0f;
    if (ImGui::BeginChild("##stage3body",
                          ImVec2(0.0f, -FooterReserveH),
                          false,
                          ImGuiWindowFlags_NoBackground)) {

        // ---- Preset cards row ------------------------------------------
        // Card order: built-ins (Quick / Fantasy) → user-saved presets →
        // "+ Save preset". Wraps to a new row every 3 cards so the row
        // grows downward as the user accumulates presets — chosen over
        // horizontal scroll because the design team's verdict pending,
        // wrap-to-row keeps the layout readable without scroll-state
        // bookkeeping (sprint-04.md Story 4.4 mitigation).
        I::DrawSectionLabel("PRESETS");
        const auto builtIns = WrapPreset::BuiltIns();
        constexpr int kCardsPerRow = 3;

        if (ImGui::BeginTable("##presetCards", kCardsPerRow,
                              ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_NoPadOuterX)) {
            int cellIdx = 0;
            auto AdvanceCell = [&]() {
                if ((cellIdx % kCardsPerRow) == 0) ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ++cellIdx;
            };

            for (const auto& preset : builtIns) {
                AdvanceCell();
                const bool active = (activePresetId_ == preset.id);
                if (DrawPresetCard(preset.id.c_str(),
                                   preset.name.c_str(),
                                   preset.description.c_str(),
                                   preset.iconGlyph.c_str(),
                                   active, /*disabled=*/false)) {
                    preset.ApplyTo(morph);
                    activePresetId_ = preset.id;
                }
            }

            for (const auto& preset : userPresets) {
                AdvanceCell();
                const bool active = (activePresetId_ == preset.id);
                if (DrawPresetCard(preset.id.c_str(),
                                   preset.name.c_str(),
                                   preset.description.empty()
                                       ? "Custom" : preset.description.c_str(),
                                   preset.iconGlyph.empty()
                                       ? MV_ICON_TARGET : preset.iconGlyph.c_str(),
                                   active, /*disabled=*/false)) {
                    preset.ApplyTo(morph);
                    activePresetId_ = preset.id;
                }
                // Right-click context menu — popup id is auto-derived from
                // the card's last item (the InvisibleButton inside
                // DrawPresetCard), so each user preset card gets its own
                // popup despite the shared MenuItem labels.
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Rename...")) {
                        out.renamePresetRequested = true;
                        out.targetPresetId = preset.id;
                    }
                    if (ImGui::MenuItem("Replace with current parameters")) {
                        out.replacePresetRequested = true;
                        out.targetPresetId = preset.id;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete")) {
                        out.deletePresetRequested = true;
                        out.targetPresetId = preset.id;
                    }
                    ImGui::EndPopup();
                }
            }

            AdvanceCell();
            if (DrawPresetCard("save", "Save preset", "Custom", MV_ICON_PLUS,
                               /*active=*/false, /*disabled=*/false)) {
                out.savePresetRequested = true;
            }

            ImGui::EndTable();
        }

        // ---- Vertex mask (Story 5.1 — promoted to top of body) --------
        // Masking is the most-used Stage 3 control once a morph is in
        // flight, so it deserves first slot per the master plan layout.
        // Existing mesh-ready guard preserved (no mesh = nothing to mask).
        const bool morphMeshReady = project.GetMorphMesh().isLoaded || morph.isProcessed;
        if (morphMeshReady) {
            I::DrawSectionLabel("VERTEX MASK");

            {
                MvStyle::ScopedFont f(MvStyle::Font.BodySm);
                ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
                ImGui::TextWrapped("Paint to exclude vertices from the next "
                                   "Process — useful for eyes, mouths, and "
                                   "interior cavities.");
                ImGui::PopStyleColor();
            }
            ImGui::Dummy(ImVec2(0.0f, 4.0f));

            ImGui::Checkbox("Paint mask", &maskPaintingActive_);
            ImGui::SameLine();
            ImGui::Checkbox("Show mask", &maskShowMask_);

            ImGui::BeginDisabled(!maskPaintingActive_);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            ImGui::SliderFloat("Mask radius", &maskBrushRadius_, 0.5f, 50.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Checkbox("Erase", &maskEraseMode_);
            ImGui::EndDisabled();

            const bool hasMaskData = !morph.vertexMask.empty();
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            ImGui::BeginDisabled(!hasMaskData);
            if (ImGui::BeginTable("##maskActions", 2,
                                  ImGuiTableFlags_SizingStretchSame |
                                  ImGuiTableFlags_NoPadOuterX)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button("Clear mask", ImVec2(-FLT_MIN, 28.0f))) {
                    out.clearMaskRequested = true;
                }
                ImGui::TableNextColumn();
                if (ImGui::Button("Invert mask", ImVec2(-FLT_MIN, 28.0f))) {
                    out.invertMaskRequested = true;
                }
                ImGui::EndTable();
            }
            ImGui::EndDisabled();

            // Mirror the current mask controls onto the primary viewport.
            out.maskPaintingActive = maskPaintingActive_;
            out.maskShow           = maskShowMask_ || maskPaintingActive_;
            out.maskEraseMode      = maskEraseMode_;
            out.maskBrushRadius    = maskBrushRadius_;
        } else {
            // Ensure the viewport doesn't treat LMB as mask painting when
            // no mesh is loaded yet.
            out.maskPaintingActive = false;
            out.maskShow           = false;
        }

        // ---- Parameter edit tracking + collapsible-section helper -----
        // Track per-frame edits across the slider blocks below so any
        // manual tweak clears activePresetId_ and the active-card
        // highlight disappears. Next frame's auto-derive at the top of
        // RenderMorph re-checks for a built-in match (handles the
        // edit-back-to-default case).
        bool paramEdited = false;

        // Story 5.1 — collapsed/expanded state persists in
        // %APPDATA%\MetaVisage\config.json under ui_state.collapsed_sections.
        // SetNextItemOpen(_, ImGuiCond_Once) seeds ImGui's internal storage
        // on the first frame; subsequent toggles are caught by comparing
        // CollapsingHeader's return value to the saved state and writing
        // the diff back to Config.
        auto DrawCollapsibleSection = [](const char* configKey,
                                         const char* label,
                                         bool defaultExpanded) -> bool {
            const bool wasOpen =
                Config::Instance().GetCollapsedSection(configKey, defaultExpanded);
            ImGui::SetNextItemOpen(wasOpen, ImGuiCond_Once);
            const bool isOpen = ImGui::CollapsingHeader(label);
            if (isOpen != wasOpen) {
                Config::Instance().SetCollapsedSection(configKey, isOpen);
            }
            return isOpen;
        };

        // ---- General (Story 5.1 — collapsible, default expanded) ------
        if (DrawCollapsibleSection("morph.general", "General",
                                   /*defaultExpanded=*/true)) {
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            if (ImGui::SliderInt("Stiffness steps",     &morph.nricpStiffnessSteps, 1, 10))      paramEdited = true;
            if (ImGui::SliderFloat("Stiffness (start)", &morph.nricpAlphaInitial,   1.0f, 500.0f, "%.1f")) paramEdited = true;
            if (ImGui::SliderFloat("Stiffness (end)",   &morph.nricpAlphaFinal,     0.1f, 200.0f, "%.1f")) paramEdited = true;
            if (ImGui::SliderInt("Iterations / step",   &morph.nricpIcpIterations,  1, 20))      paramEdited = true;
            if (ImGui::SliderFloat("Normal threshold",  &morph.nricpNormalThreshold, 5.0f, 120.0f, "%.0f\xC2\xB0")) paramEdited = true;
            if (ImGui::SliderFloat("Landmark weight",   &morph.nricpLandmarkWeight, 0.0f, 100.0f, "%.1f")) paramEdited = true;
            ImGui::PopItemWidth();

            // Interior Geometry sub-section
            I::DrawSubSectionLabel("INTERIOR GEOMETRY");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            if (ImGui::Checkbox("Exclude mesh boundaries",
                                &morph.nricpEnableBoundaryExclusion)) paramEdited = true;
            ImGui::BeginDisabled(!morph.nricpEnableBoundaryExclusion);
            if (ImGui::SliderInt("Boundary hops",
                                 &morph.nricpBoundaryExclusionHops, 0, 10)) paramEdited = true;
            ImGui::EndDisabled();
            ImGui::PopItemWidth();
        }

        // ---- Advanced (Story 5.1 — three sub-sections, default closed) -
        if (DrawCollapsibleSection("morph.advanced", "Advanced",
                                   /*defaultExpanded=*/false)) {
            // Optimization sub-section
            I::DrawSubSectionLabel("OPTIMIZATION");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            if (ImGui::SliderInt("Inner iterations",       &morph.nricpOptimizationIterations, 1, 10)) paramEdited = true;
            if (ImGui::SliderFloat("Step damping (start)", &morph.nricpDpInitial,              0.1f, 1.0f, "%.2f")) paramEdited = true;
            if (ImGui::SliderFloat("Step damping (end)",   &morph.nricpDpFinal,                0.1f, 1.0f, "%.2f")) paramEdited = true;
            if (ImGui::InputFloat("Convergence threshold", &morph.nricpEpsilon, 0.0f, 0.0f, "%.5f")) paramEdited = true;
            ImGui::PopItemWidth();

            // Rigidity sub-section
            I::DrawSubSectionLabel("RIGIDITY");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            if (ImGui::SliderFloat("Rigidity (start)", &morph.nricpGammaInitial, 0.0f, 5.0f, "%.2f")) paramEdited = true;
            if (ImGui::SliderFloat("Rigidity (end)",   &morph.nricpGammaFinal,   0.0f, 5.0f, "%.2f")) paramEdited = true;
            ImGui::PopItemWidth();

            // Control Node Sampling sub-section
            I::DrawSubSectionLabel("CONTROL NODE SAMPLING");
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
            if (ImGui::SliderFloat("Control density (start)", &morph.nricpSamplingInitial, 0.0f, 1.0f, "%.3f")) paramEdited = true;
            if (ImGui::SliderFloat("Control density (end)",   &morph.nricpSamplingFinal,   0.0f, 1.0f, "%.3f")) paramEdited = true;
            if (ImGui::Checkbox("Normalize sampling",         &morph.nricpNormalizeSampling)) paramEdited = true;
            ImGui::PopItemWidth();
        }

        // ---- Reset to Defaults (Story 5.1 — relocated outside Advanced) -
        // Previously hidden inside the collapsed Advanced section; now
        // always visible so a tweaked-too-far parameter set is one click
        // from a known-good baseline regardless of which sections are open.
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        if (ImGui::Button("Reset to defaults", ImVec2(-FLT_MIN, 30.0f))) {
            morph = MorphData();
            activePresetId_.clear();
        }

        // ---- Cottage-cheese-face stopgap (Story 5.2 Outcome B) ---------
        // High landmark weight relative to final stiffness leaves the
        // landmark term over-weighted at the last stiffness level, which
        // matches the reported local-stretching artifact. Threshold lives
        // in the anonymous namespace at the top of this file — PR #24
        // review nit so the follow-up root-cause spike can grep for it.
        // The warning is informational — Process is not blocked.
        const float ratio = morph.nricpAlphaFinal > 0.0f
            ? morph.nricpLandmarkWeight / morph.nricpAlphaFinal
            : 0.0f;
        if (ratio > kLandmarkRatioWarnThreshold) {
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvWarning);
            ImGui::TextWrapped(
                MV_ICON_ALERT
                "  High landmark weight relative to final stiffness can "
                "cause local geometry artifacts. Consider lowering "
                "Landmark Weight or raising Stiffness (end).");
            ImGui::PopStyleColor();
        }

        if (paramEdited) activePresetId_.clear();

        // ---- Process / cancel ------------------------------------------
        I::DrawSectionLabel("PROCESS");
        if (processing) {
            char label[64];
            std::snprintf(label, sizeof(label), "Processing\xE2\x80\xA6 %.0f%%",
                          progress * 100.0f);
            ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0), label);
            if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 30.0f))) {
                out.cancelMorphRequested = true;
            }
        } else {
            const bool canProcess = project.GetMorphMesh().isLoaded
                                 && project.GetTargetMesh().isLoaded;
            ImGui::BeginDisabled(!canProcess);
            // Primary blue Process button — the main action of the stage.
            ImGui::PushStyleColor(ImGuiCol_Button,        MvStyle::MvBlue500);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, MvStyle::MvBlue400);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  MvStyle::MvBlue600);
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0x05/255.f, 0x13/255.f, 0x22/255.f, 1.f));
            if (ImGui::Button("Process Morph", ImVec2(-FLT_MIN, 32.0f))) {
                out.processMorphRequested = true;
            }
            ImGui::PopStyleColor(4);
            ImGui::EndDisabled();
            if (!canProcess) {
                MvStyle::ScopedFont f(MvStyle::Font.BodySm);
                ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
                ImGui::TextWrapped("Load both meshes before processing.");
                ImGui::PopStyleColor();
            }
        }

        // ---- Inline result summary -------------------------------------
        // The viewport stats overlay surfaces the same numbers in the GL
        // pane; the inspector still echoes them here so users with the
        // overlay hidden (e.g. via dual-monitor split) still see them.
        if (morph.isProcessed) {
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            MvStyle::ScopedFont f(MvStyle::Font.Mono);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
            ImGui::Text("Max disp.  %.3f mm", morph.maxDisplacement);
            ImGui::Text("Avg disp.  %.3f mm", morph.avgDisplacement);
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
    }
    ImGui::EndChild();

    // ---- Footer ---------------------------------------------------------
    // Continue button labelled "Accept" per design. Enabled once the morph
    // worker has produced a result; clicking it flags the result accepted
    // (so Project::CanProceedToNextStage returns true) AND requests the
    // stage advance — AppWindow processes both this frame.
    const bool canAccept = morph.isProcessed;
    const auto footer = I::DrawInspectorFooter(
        "STEP 03 OF 04",
        /*previousEnabled=*/true,
        /*continueEnabled=*/canAccept,
        /*continueLabel=*/morph.isAccepted ? "Continue" : "Accept",
        /*finishStyle=*/false);
    if (footer.previousClicked) out.previousStageRequested = true;
    if (footer.continueClicked) {
        if (!morph.isAccepted) out.acceptMorphRequested = true;
        out.nextStageRequested = true;
    }
}

// ---------------------------------------------------------------------------
// Stage 4 — Touch Up
// ---------------------------------------------------------------------------

void StageSidebar::RenderTouchUp(Project& project, Actions& out) {
    namespace I = MvInspector;
    auto& sculpt = project.GetSculptState();

    // ---- Header ---------------------------------------------------------
    I::DrawStageHeader("STAGE 04 / 04",
                       "Touch ", "up",
                       "Sculpt the deformed mesh to refine the wrap. "
                       "Pick a brush and scrub the surface; bracket keys "
                       "adjust radius, Shift smooths.");

    // ---- Scrollable body ------------------------------------------------
    constexpr float FooterReserveH = 60.0f;
    if (ImGui::BeginChild("##stage4body",
                          ImVec2(0.0f, -FooterReserveH),
                          false,
                          ImGuiWindowFlags_NoBackground)) {

        // ---- BRUSHES ----------------------------------------------------
        I::DrawSectionLabel("SCULPT BRUSHES");

        struct BrushDef {
            BrushType   kind;
            const char* name;
            const char* hotkey;
            const char* glyph;
            const char* description;
        };
        // Hotkey marks mirror Viewport::OnKey without adding labels to the
        // compact palette.
        static const BrushDef kBrushes[4] = {
            { BrushType::Smooth,   "Smooth",    "1", MV_ICON_BRUSH,
              "Average nearby vertices to soften small surface artifacts." },
            { BrushType::Grab,     "Grab",      "2", MV_ICON_HAND,
              "Drag the surface with the cursor for free-form touch-up passes." },
            { BrushType::PushPull, "Push/Pull", "3", MV_ICON_MOVE_VERTICAL,
              "Push or pull vertices along the surface normal." },
            { BrushType::Inflate,  "Inflate",   "4", MV_ICON_EXPAND,
              "Expand vertices radially to add volume without rebuilding the morph." },
        };

        if (ImGui::BeginTable("##brushGrid", 4,
                              ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_NoPadOuterX)) {
            for (size_t i = 0; i < 4; ++i) {
                if (i % 4 == 0) ImGui::TableNextRow();
                ImGui::TableNextColumn();
                const bool active = (sculpt.activeBrush == kBrushes[i].kind);
                const ImTextureID thumb =
                    (ImTextureID)(intptr_t)MvStyle::Asset.BrushPreviewTextureIds[i];
                if (DrawBrushPaletteTile(kBrushes[i].name,
                                         kBrushes[i].name,
                                         kBrushes[i].glyph,
                                         kBrushes[i].hotkey,
                                         active,
                                         thumb)) {
                    sculpt.activeBrush = kBrushes[i].kind;
                }
            }
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        // ---- SETTINGS — brush radius / strength / falloff -------------
        I::DrawSectionLabel("SETTINGS");

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.62f);
        ImGui::SliderFloat("Radius",   &sculpt.brushRadius,   1.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Strength", &sculpt.brushStrength, 0.0f, 1.0f,   "%.2f");

        int falloffIdx = static_cast<int>(sculpt.falloff);
        const char* falloffs[] = { "Linear", "Smooth", "Sharp" };
        if (ImGui::Combo("Falloff", &falloffIdx, falloffs, 3)) {
            sculpt.falloff = static_cast<FalloffType>(falloffIdx);
        }
        ImGui::PopItemWidth();

        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        // Symmetry — checkbox + axis combo on one row.
        ImGui::Checkbox("Symmetry", &sculpt.symmetryEnabled);
        ImGui::SameLine();
        ImGui::BeginDisabled(!sculpt.symmetryEnabled);
        ImGui::PushItemWidth(80.0f);
        int axisIdx = AxisToIndex(sculpt.symmetryAxis);
        const char* axes[] = { "X", "Y", "Z" };
        if (ImGui::Combo("##sym_axis", &axisIdx, axes, 3)) {
            sculpt.symmetryAxis = IndexToAxis(axisIdx);
        }
        ImGui::PopItemWidth();
        ImGui::EndDisabled();

        ImGui::Checkbox("Show target overlay", &sculpt.showTargetOverlay);

        // ---- Export Mesh button — full-width primary green ------------
        // Echoes the green Finish button in the footer below; users
        // expect a discoverable export entry point inside the panel
        // body (the v1.0-era flow), the footer is for stage navigation
        // semantics. Both fire the same Actions::exportMeshRequested.
        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        ImGui::PushStyleColor(ImGuiCol_Button,        MvStyle::MvSuccess);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(MvStyle::MvSuccess.x, MvStyle::MvSuccess.y,
                                     MvStyle::MvSuccess.z, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(MvStyle::MvSuccess.x, MvStyle::MvSuccess.y,
                                     MvStyle::MvSuccess.z, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(0x05/255.f, 0x13/255.f, 0x22/255.f, 1.f));
        if (ImGui::Button("Export Mesh\xE2\x80\xA6", ImVec2(-FLT_MIN, 36.0f))) {
            out.exportMeshRequested = true;
        }
        ImGui::PopStyleColor(4);

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
    }
    ImGui::EndChild();

    // ---- Footer ---------------------------------------------------------
    // Stage 4 is the terminal stage — Continue is relabelled "Finish" and
    // styled green; clicking it triggers the same Export Mesh flow that
    // the inline button above hits.
    const auto footer = I::DrawInspectorFooter(
        "STEP 04 OF 04",
        /*previousEnabled=*/true,
        /*continueEnabled=*/true,
        /*continueLabel=*/"Finish",
        /*finishStyle=*/true);
    if (footer.previousClicked) out.previousStageRequested = true;
    if (footer.continueClicked) out.exportMeshRequested = true;
}

} // namespace MetaVisage
