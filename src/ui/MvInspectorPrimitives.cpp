#include "ui/MvInspectorPrimitives.h"

#include "ui/MvIcons.h"
#include "ui/MvStyle.h"

#include <imgui.h>

#include <cstdio>
#include <cstring>

namespace MetaVisage::MvInspector {

namespace {

inline ImU32 ColU(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

inline ImVec4 WithAlpha(const ImVec4& v, float a) {
    return ImVec4(v.x, v.y, v.z, a);
}

} // namespace

// =============================================================================
// 1. Stage header
// =============================================================================
void DrawStageHeader(const char* eyebrow,
                     const char* baseTitle,
                     const char* accentSuffix,
                     const char* description) {
    // Eyebrow — MonoSm uppercase, MvBlue400, 0.16em tracking is achieved
    // visually via the font's metrics; we don't manually letter-space.
    if (eyebrow && eyebrow[0]) {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvBlue400);
        ImGui::TextUnformatted(eyebrow);
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    // Display heading — Saira ExtraBold Italic. Base portion in MvFg1, accent
    // suffix in MvBlue500. Rendered as two SameLine spans so the colour
    // boundary stays glued without a space.
    if ((baseTitle && baseTitle[0]) || (accentSuffix && accentSuffix[0])) {
        MvStyle::ScopedFont f(MvStyle::Font.Display);
        if (baseTitle && baseTitle[0]) {
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg1);
            ImGui::TextUnformatted(baseTitle);
            ImGui::PopStyleColor();
            if (accentSuffix && accentSuffix[0]) {
                ImGui::SameLine(0.0f, 0.0f);
            }
        }
        if (accentSuffix && accentSuffix[0]) {
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvBlue500);
            ImGui::TextUnformatted(accentSuffix);
            ImGui::PopStyleColor();
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    // Description — BodySm, MvFg2, soft-wrapped to the panel width.
    if (description && description[0]) {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
        ImGui::TextWrapped("%s", description);
        ImGui::PopStyleColor();
    }
}

// =============================================================================
// 2. Section label
// =============================================================================
void DrawSectionLabel(const char* title) {
    if (!title || !title[0]) return;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Hairline rule above — full panel width.
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    {
        const ImVec2 cur = ImGui::GetCursorScreenPos();
        const float w = ImGui::GetContentRegionAvail().x;
        dl->AddLine(cur, ImVec2(cur.x + w, cur.y),
                    ColU(MvStyle::MvLine2), 1.0f);
    }
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    // 6×6 MvBlue500 square dot + mono uppercase label.
    constexpr float DotSize = 6.0f;
    const ImVec2 cur = ImGui::GetCursorScreenPos();
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        const float fs = ImGui::GetFontSize();
        const float dotY = cur.y + (fs - DotSize) * 0.5f + 1.0f;
        dl->AddRectFilled(ImVec2(cur.x,            dotY),
                          ImVec2(cur.x + DotSize,  dotY + DotSize),
                          ColU(MvStyle::MvBlue500));

        ImGui::Dummy(ImVec2(DotSize + 8.0f, fs));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
    }


    ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

// =============================================================================
// 2b. Sub-section label — Story 5.1
// =============================================================================
void DrawSubSectionLabel(const char* title) {
    if (!title || !title[0]) return;

    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    constexpr float SubLabelIndent = 10.0f;
    ImGui::Indent(SubLabelIndent);
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
    }
    ImGui::Unindent(SubLabelIndent);

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
}

// =============================================================================
// 3. File slot
// =============================================================================
FileSlotResult DrawFileSlot(const char* filename,
                            const char* placeholder,
                            bool isLoaded,
                            bool isPrimary,
                            const char* fileTypeLabel,
                            const char* importLabel) {
    FileSlotResult result;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Scope the Import button's ID — callers typically render multiple file
    // slots back-to-back with the same "Import" label, which would collapse
    // to one ImGui widget ID and trigger the conflicting-ID assert. Use the
    // placeholder as the discriminator (each call site passes a unique one
    // like "No source mesh" / "No target mesh"); fall back to a literal so
    // a null placeholder still pushes a valid scope.
    ImGui::PushID(placeholder ? placeholder : "##fileSlot");

    // Container — MvBg1 + MvLine2 border, rounded 6, padding 10×12. Drawn
    // manually so the folder icon aligns with the filename baseline.
    const ImVec2 boxMin = ImGui::GetCursorScreenPos();
    const float boxW = ImGui::GetContentRegionAvail().x;
    const float boxH = (isLoaded && fileTypeLabel && fileTypeLabel[0]) ? 50.0f : 40.0f;
    const ImVec2 boxMax(boxMin.x + boxW, boxMin.y + boxH);

    dl->AddRectFilled(boxMin, boxMax, ColU(MvStyle::MvBg1), 6.0f);
    dl->AddRect(boxMin, boxMax, ColU(MvStyle::MvLine2), 6.0f, 0, 1.0f);

    // 30×30 folder-icon square — tinted blue when loaded, MvFg4 when not.
    constexpr float IconSlot = 30.0f;
    const ImVec2 iconMin(boxMin.x + 10.0f, boxMin.y + (boxH - IconSlot) * 0.5f);
    const ImVec2 iconMax(iconMin.x + IconSlot, iconMin.y + IconSlot);
    const ImU32 iconBg = isLoaded
        ? ColU(WithAlpha(MvStyle::MvBlue500, 0.12f))
        : ColU(MvStyle::MvBg2);
    dl->AddRectFilled(iconMin, iconMax, iconBg, 4.0f);

    if (MvStyle::Font.Icon) {
        ImFont* iconFont = MvStyle::Font.Icon;
        const float iconSize = 16.0f;
        const ImVec2 ts = iconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, MV_ICON_FOLDER);
        const ImVec2 textPos((iconMin.x + iconMax.x - ts.x) * 0.5f,
                             (iconMin.y + iconMax.y - ts.y) * 0.5f);
        const ImU32 iconCol = isLoaded
            ? ColU(MvStyle::MvBlue400)
            : ColU(MvStyle::MvFg4);
        dl->AddText(iconFont, iconSize, textPos, iconCol, MV_ICON_FOLDER);
    }

    // Filename / placeholder + optional file-type sub-line.
    const float textX = iconMax.x + 10.0f;
    {
        MvStyle::ScopedFont f(MvStyle::Font.Mono);
        const float fs = ImGui::GetFontSize();
        const bool showSub = isLoaded && fileTypeLabel && fileTypeLabel[0];
        const float nameY = showSub
            ? boxMin.y + 10.0f
            : boxMin.y + (boxH - fs) * 0.5f;

        const char* mainText = (filename && filename[0]) ? filename
                                                          : (placeholder ? placeholder : "");
        const ImU32 mainCol = (filename && filename[0])
            ? ColU(MvStyle::MvFg1)
            : ColU(MvStyle::MvFg3);

        // Clip to the available space so a long filename doesn't bleed past
        // the box. ImDrawList::AddText doesn't auto-truncate, so we push a
        // clip rect for this span.
        const float textRight = boxMax.x - 10.0f;
        dl->PushClipRect(ImVec2(textX, boxMin.y), ImVec2(textRight, boxMax.y), true);
        dl->AddText(ImVec2(textX, nameY), mainCol, mainText);
        dl->PopClipRect();

        if (showSub) {
            MvStyle::ScopedFont f2(MvStyle::Font.MonoSm);
            const float fs2 = ImGui::GetFontSize();
            const float subY = boxMin.y + boxH - fs2 - 8.0f;
            dl->AddText(ImVec2(textX, subY), ColU(MvStyle::MvFg3), fileTypeLabel);
        }
    }

    // Reserve the box's vertical footprint so the Import button below sits
    // flush. Then advance.
    ImGui::Dummy(ImVec2(boxW, boxH));
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    // Import button — full width. Primary blue (isPrimary) or ghost.
    if (isPrimary) {
        ImGui::PushStyleColor(ImGuiCol_Button,        MvStyle::MvBlue500);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, MvStyle::MvBlue400);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  MvStyle::MvBlue600);
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0x05/255.f, 0x13/255.f, 0x22/255.f, 1.f));
    }
    if (ImGui::Button(importLabel, ImVec2(-FLT_MIN, 32.0f))) {
        result.importClicked = true;
    }
    if (isPrimary) ImGui::PopStyleColor(4);

    ImGui::PopID();
    return result;
}

// =============================================================================
// 4. Axis input — custom drag widget with right-aligned mono value.
// =============================================================================
DragEditResult DrawAxisInput(const char* axisLetter,
                             const ImVec4& axisColor,
                             float& valueInOut,
                             float dragSpeed,
                             bool disabled) {
    DragEditResult result;
    constexpr float FrameH = 28.0f;
    const float availW = ImGui::GetContentRegionAvail().x;

    ImGui::PushID(axisLetter);

    // Sprint 9.3 — typed entry. Double-click or Ctrl+click swaps the drag
    // field for an InputFloat; Enter / focus loss commits, Esc cancels. The
    // whole typing session is one gesture: editStarted fires on the click
    // that opened it, editFinished when it closes, so callers keep their
    // one-undo-per-gesture contract. State lives in ImGui storage keyed by
    // this widget's ID stack, so no caller-side bookkeeping is needed.
    enum TypingState : int { NotTyping = 0, FocusPending = 1, Typing = 2 };
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID stateKey = ImGui::GetID("##typingState");
    const ImGuiID valueKey = ImGui::GetID("##typingValue");
    const ImGuiID frameKey = ImGui::GetID("##typingOpenedFrame");
    const int typingState = disabled ? NotTyping : storage->GetInt(stateKey, NotTyping);

    if (typingState != NotTyping) {
        float typed = storage->GetFloat(valueKey, valueInOut);
        if (typingState == FocusPending) ImGui::SetKeyboardFocusHere();

        MvStyle::ScopedFont f(MvStyle::Font.Mono);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2(8.0f, (FrameH - ImGui::GetFontSize()) * 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, MvStyle::MvBg3);
        ImGui::PushStyleColor(ImGuiCol_Border,  MvStyle::MvBlue500);
        ImGui::SetNextItemWidth(availW);
        ImGui::InputFloat("##axisType", &typed, 0.0f, 0.0f, "%.2f",
                          ImGuiInputTextFlags_AutoSelectAll);
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        // Close if focus never lands (e.g. the window lost focus), so the
        // field can't get stuck in typing mode.
        const bool focusTimedOut = typingState == FocusPending &&
            !ImGui::IsItemActive() &&
            ImGui::GetFrameCount() - storage->GetInt(frameKey, 0) > 3;
        const bool cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
        const bool closed = ImGui::IsItemDeactivated() || focusTimedOut ||
                            (typingState == Typing && !ImGui::IsItemActive());
        if (closed || cancelled) {
            if (!cancelled && typed != valueInOut) {
                valueInOut = typed;
                result.changed = true;
            }
            result.editFinished = true;
            storage->SetInt(stateKey, NotTyping);
        } else {
            storage->SetFloat(valueKey, typed);
            if (ImGui::IsItemActive()) storage->SetInt(stateKey, Typing);
        }
        ImGui::PopID();
        return result;
    }

    const ImVec2 framePos = ImGui::GetCursorScreenPos();
    const ImVec2 frameMax(framePos.x + availW, framePos.y + FrameH);

    // ImGui::DragFloat doesn't right-align text and the framing wouldn't
    // exactly match the design either, so we build a custom widget on top
    // of an InvisibleButton. The InvisibleButton owns the hit-test and the
    // active-drag state; we paint the frame, axis letter, and value.
    bool hovered = false;
    bool active  = false;

    if (disabled) {
        ImGui::BeginDisabled();
    }
    ImGui::InvisibleButton("##axisDrag", ImVec2(availW, FrameH));
    hovered = ImGui::IsItemHovered();
    active  = ImGui::IsItemActive();
    result.editStarted  = ImGui::IsItemActivated();
    result.editFinished = ImGui::IsItemDeactivated();
    if (!disabled) {
        ImGui::SetItemTooltip("Drag to adjust. Double-click or Ctrl+click to type.");
    }
    if (disabled) {
        ImGui::EndDisabled();
    }

    const bool openTyping = !disabled && result.editStarted &&
        (ImGui::GetIO().KeyCtrl || ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left));
    if (openTyping) {
        // The click that opened typing stays this gesture's editStarted;
        // the drag button stops being submitted, so suppress its delta.
        storage->SetInt(stateKey, FocusPending);
        storage->SetFloat(valueKey, valueInOut);
        storage->SetInt(frameKey, ImGui::GetFrameCount());
        active = false;
    }

    if (active && !disabled) {
        const float dx = ImGui::GetIO().MouseDelta.x;
        if (dx != 0.0f) {
            valueInOut += dx * dragSpeed;
            result.changed = true;
        }
    }
    if (hovered && !disabled) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    // Draw frame
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float dimAlpha = disabled ? 0.5f : 1.0f;
    const ImU32 bgCol = (active && !disabled) ? ColU(WithAlpha(MvStyle::MvBg3, dimAlpha))
                      : (hovered && !disabled) ? ColU(WithAlpha(MvStyle::MvBg2, dimAlpha))
                                               : ColU(WithAlpha(MvStyle::MvBg1, dimAlpha));
    dl->AddRectFilled(framePos, frameMax, bgCol, 4.0f);
    dl->AddRect(framePos, frameMax,
                ColU(WithAlpha(MvStyle::MvLine2, dimAlpha)),
                4.0f, 0, 1.0f);

    // Axis letter — MonoSm bold-ish, in axisColor.
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        const float fs = ImGui::GetFontSize();
        dl->AddText(ImVec2(framePos.x + 8.0f,
                           framePos.y + (FrameH - fs) * 0.5f),
                    ColU(WithAlpha(axisColor, dimAlpha)),
                    axisLetter);
    }

    // Value — Mono, MvFg1, right-aligned with 8px right pad.
    {
        MvStyle::ScopedFont f(MvStyle::Font.Mono);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", valueInOut);
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        const float fs = ImGui::GetFontSize();
        dl->AddText(ImVec2(frameMax.x - ts.x - 8.0f,
                           framePos.y + (FrameH - fs) * 0.5f),
                    ColU(WithAlpha(MvStyle::MvFg1, dimAlpha)),
                    buf);
    }

    ImGui::PopID();
    return result;
}

// =============================================================================
// 5. Transform group
// =============================================================================
DragEditResult DrawTransformGroup(const char* label,
                                  const char* unit,
                                  TransformValues3& valuesInOut,
                                  bool uniform,
                                  float dragSpeed) {
    DragEditResult result;

    // Label row: "Position" on the left, "cm · uniform" on the right (mono,
    // MvFg4 sub-tone for the unit).
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg2);
        ImGui::TextUnformatted(label ? label : "");
        ImGui::PopStyleColor();
    }
    if ((unit && unit[0]) || uniform) {
        ImGui::SameLine();
        char rightBuf[32];
        std::snprintf(rightBuf, sizeof(rightBuf), "%s%s",
                      unit ? unit : "",
                      uniform ? (unit && unit[0] ? " \xC2\xB7 uniform" : "uniform") : "");
        const float availW = ImGui::GetContentRegionAvail().x;
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        const ImVec2 ts = ImGui::CalcTextSize(rightBuf);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - ts.x));
        ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg4);
        ImGui::TextUnformatted(rightBuf);
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    // 3-column grid via BeginTable — predictable equal-width columns
    // regardless of font load state.
    const ImGuiTableFlags tableFlags =
        ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadInnerX |
        ImGuiTableFlags_NoPadOuterX;
    // Scope axis IDs per group so Position / Rotation / Scale "X" fields
    // never share drag or typing state.
    ImGui::PushID(label ? label : "");
    if (ImGui::BeginTable("##xform", 3, tableFlags)) {
        ImGui::TableNextRow();

        // Aggregate per-axis edit signals — the caller wants a single
        // editStarted/editFinished pair per drag gesture, regardless of
        // which axis the user grabbed.
        auto merge = [&](const DragEditResult& r) {
            if (r.changed)      result.changed      = true;
            if (r.editStarted)  result.editStarted  = true;
            if (r.editFinished) result.editFinished = true;
        };

        ImGui::TableNextColumn();
        const auto rx = DrawAxisInput("X", MvStyle::MvAxisX,
                                      valuesInOut.x, dragSpeed, /*disabled=*/false);
        merge(rx);
        if (rx.changed && uniform) {
            valuesInOut.y = valuesInOut.x;
            valuesInOut.z = valuesInOut.x;
        }

        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(2.0f, 0.0f));
        ImGui::SameLine();
        merge(DrawAxisInput("Y", MvStyle::MvAxisY,
                            valuesInOut.y, dragSpeed, /*disabled=*/uniform));

        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(2.0f, 0.0f));
        ImGui::SameLine();
        merge(DrawAxisInput("Z", MvStyle::MvAxisZ,
                            valuesInOut.z, dragSpeed, /*disabled=*/uniform));

        ImGui::EndTable();
    }
    ImGui::PopID();
    return result;
}

// =============================================================================
// 6. Validation row
// =============================================================================
void DrawValidationRow(const char* label,
                       const char* value,
                       ValidationStatus status) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec4 statusColor;
    const char* statusGlyph;
    switch (status) {
        case ValidationStatus::Ok:      statusColor = MvStyle::MvSuccess; statusGlyph = MV_ICON_CHECK_CIRCLE; break;
        case ValidationStatus::Warning: statusColor = MvStyle::MvWarning; statusGlyph = MV_ICON_ALERT;        break;
        case ValidationStatus::Error:
        default:                        statusColor = MvStyle::MvDanger;  statusGlyph = "\xEE\x82\x84";       break; // U+E084 circle-x
    }

    constexpr float RowH    = 22.0f;
    constexpr float BadgeSz = 14.0f;

    const ImVec2 rowStart = ImGui::GetCursorScreenPos();
    const float availW = ImGui::GetContentRegionAvail().x;

    // Status badge — small Lucide glyph at MvSuccess/MvWarning/MvDanger.
    if (MvStyle::Font.IconSm) {
        ImFont* iconFont = MvStyle::Font.IconSm;
        const float iconSize = 13.0f;
        const ImVec2 ts = iconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, statusGlyph);
        const ImVec2 textPos(rowStart.x + (BadgeSz - ts.x) * 0.5f,
                             rowStart.y + (RowH - ts.y) * 0.5f);
        dl->AddText(iconFont, iconSize, textPos, ColU(statusColor), statusGlyph);
    }

    // Label — body, MvFg2.
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        const float fs = ImGui::GetFontSize();
        dl->AddText(ImVec2(rowStart.x + BadgeSz + 8.0f,
                           rowStart.y + (RowH - fs) * 0.5f),
                    ColU(MvStyle::MvFg2),
                    label ? label : "");
    }

    // Value — mono, MvFg1, right-aligned.
    if (value && value[0]) {
        MvStyle::ScopedFont f(MvStyle::Font.Mono);
        const ImVec2 ts = ImGui::CalcTextSize(value);
        const float fs = ImGui::GetFontSize();
        dl->AddText(ImVec2(rowStart.x + availW - ts.x,
                           rowStart.y + (RowH - fs) * 0.5f),
                    ColU(MvStyle::MvFg1),
                    value);
    }

    ImGui::Dummy(ImVec2(availW, RowH));
}

// =============================================================================
// 7. Inspector footer
// =============================================================================
InspectorFooterResult DrawInspectorFooter(const char* stepLabel,
                                           bool previousEnabled,
                                           bool continueEnabled,
                                           const char* continueLabel,
                                           bool finishStyle) {
    InspectorFooterResult result;
    if (!continueLabel) continueLabel = "Continue";

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1 px MvLine2 hairline above the footer row.
    {
        const ImVec2 cur = ImGui::GetCursorScreenPos();
        const float w = ImGui::GetContentRegionAvail().x;
        dl->AddLine(cur, ImVec2(cur.x + w, cur.y),
                    ColU(MvStyle::MvLine2), 1.0f);
    }
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    // 3-column row: Previous (left, ghost) / step counter (centred mono) /
    // Continue (right, primary or success). BeginTable ensures the centre
    // counter stays put even if either button label changes width.
    const ImGuiTableFlags tableFlags =
        ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadInnerX |
        ImGuiTableFlags_NoPadOuterX;
    if (ImGui::BeginTable("##footer", 3, tableFlags)) {
        ImGui::TableNextRow();

        // Left — Previous button. Ghost style; disabled when on the first
        // stage (the caller passes previousEnabled=false there).
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(!previousEnabled);
        if (ImGui::Button("\xE2\x80\xB9 Previous", ImVec2(0.0f, 30.0f))) {
            result.previousClicked = true;
        }
        ImGui::EndDisabled();

        // Centre — mono step counter ("STEP 01 OF 04"), centred in column.
        ImGui::TableNextColumn();
        if (stepLabel && stepLabel[0]) {
            const float colW = ImGui::GetContentRegionAvail().x;
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            const ImVec2 ts = ImGui::CalcTextSize(stepLabel);
            const float fs = ImGui::GetFontSize();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (colW - ts.x) * 0.5f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (30.0f - fs) * 0.5f);
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvFg3);
            ImGui::TextUnformatted(stepLabel);
            ImGui::PopStyleColor();
        }

        // Right — Continue button. Primary blue or finish-style green;
        // right-aligned within its column so it hugs the inspector edge.
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(!continueEnabled);

        const ImVec4 baseCol  = finishStyle ? MvStyle::MvSuccess : MvStyle::MvBlue500;
        const ImVec4 hoverCol = finishStyle ? WithAlpha(MvStyle::MvSuccess, 0.85f)
                                             : MvStyle::MvBlue400;
        const ImVec4 activeCol= finishStyle ? WithAlpha(MvStyle::MvSuccess, 0.70f)
                                             : MvStyle::MvBlue600;
        ImGui::PushStyleColor(ImGuiCol_Button,        baseCol);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverCol);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  activeCol);
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0x05/255.f, 0x13/255.f, 0x22/255.f, 1.f));

        char btnLabel[64];
        std::snprintf(btnLabel, sizeof(btnLabel), "%s \xE2\x80\xBA", continueLabel);
        const ImVec2 btnSize = ImGui::CalcTextSize(btnLabel);
        const float btnW = btnSize.x + 24.0f;
        const float colW = ImGui::GetContentRegionAvail().x;
        if (colW > btnW) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (colW - btnW));
        }
        if (ImGui::Button(btnLabel, ImVec2(btnW, 30.0f))) {
            result.continueClicked = true;
        }

        ImGui::PopStyleColor(4);
        ImGui::EndDisabled();

        ImGui::EndTable();
    }
    return result;
}

} // namespace MetaVisage::MvInspector
