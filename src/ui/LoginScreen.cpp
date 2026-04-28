#include "ui/LoginScreen.h"

#include "ui/MvIcons.h"
#include "ui/MvStyle.h"

#include <imgui.h>

#include <cmath>
#include <cstdio>
#include <numbers>

namespace MetaVisage::LoginScreen {

namespace {

constexpr float kPi = std::numbers::pi_v<float>;

inline ImU32 ColU(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }

inline ImVec4 WithAlpha(const ImVec4& v, float a) {
    return ImVec4(v.x, v.y, v.z, a);
}

// Two-tone blueprint grid — the same idiom the design's CSS .mv-blueprint
// uses (96 px coarse + 16 px fine), painted directly into the full-screen
// background.
void DrawBlueprintGrid(ImDrawList* dl, ImVec2 pmin, ImVec2 pmax) {
    const ImU32 coarse = ColU(MvStyle::MvLine1);
    const ImU32 fine   = ColU(WithAlpha(MvStyle::MvLine1, 0.04f));

    for (float x = pmin.x; x < pmax.x; x += 16.0f) {
        dl->AddLine(ImVec2(x, pmin.y), ImVec2(x, pmax.y), fine, 1.0f);
    }
    for (float y = pmin.y; y < pmax.y; y += 16.0f) {
        dl->AddLine(ImVec2(pmin.x, y), ImVec2(pmax.x, y), fine, 1.0f);
    }
    for (float x = pmin.x; x < pmax.x; x += 96.0f) {
        dl->AddLine(ImVec2(x, pmin.y), ImVec2(x, pmax.y), coarse, 1.0f);
    }
    for (float y = pmin.y; y < pmax.y; y += 96.0f) {
        dl->AddLine(ImVec2(pmin.x, y), ImVec2(pmax.x, y), coarse, 1.0f);
    }
}

// L-shaped corner bracket, 16 px arm length, drawn at (cx, cy) with the
// arms extending toward the centre of the viewport (orientation flipped
// per quadrant).
void DrawCornerBracket(ImDrawList* dl, ImVec2 origin, bool flipH, bool flipV) {
    constexpr float Arm = 16.0f;
    const ImU32 col = ColU(MvStyle::MvLine3);
    const float dx = flipH ? -Arm : Arm;
    const float dy = flipV ? -Arm : Arm;
    dl->AddLine(origin, ImVec2(origin.x + dx, origin.y), col, 1.0f);
    dl->AddLine(origin, ImVec2(origin.x, origin.y + dy), col, 1.0f);
}

// Animated arc segment that sits over the auth-card hero in the Waiting
// state — same visual idiom as the design's spinner.
void DrawSpinnerRing(ImDrawList* dl, ImVec2 center, float radius,
                     double tickSeconds) {
    const float startAngle =
        static_cast<float>(std::fmod(tickSeconds * 1.6, kPi * 2.0));
    constexpr float ArcSpan = kPi * 0.55f;
    constexpr int N = 32;
    const ImU32 trackCol = ColU(WithAlpha(MvStyle::MvLine2, 0.6f));
    const ImU32 arcCol   = ColU(MvStyle::MvBlue400);

    // Faint background ring so the spinner reads as a track + sweep.
    dl->AddCircle(center, radius, trackCol, N, 1.5f);

    dl->PathClear();
    for (int i = 0; i <= N; ++i) {
        const float t = static_cast<float>(i) / N;
        const float angle = startAngle + t * ArcSpan;
        dl->PathLineTo(ImVec2(center.x + std::cos(angle) * radius,
                              center.y + std::sin(angle) * radius));
    }
    dl->PathStroke(arcCol, ImDrawFlags_None, 2.0f);
}

// "01" / "02" / "03" square badge for the idle-state explainer rows.
void DrawStepBadge(ImDrawList* dl, ImVec2 pos, const char* text) {
    constexpr float Size = 24.0f;
    const ImVec2 pmax(pos.x + Size, pos.y + Size);
    dl->AddRectFilled(pos, pmax, ColU(MvStyle::MvBg1), 4.0f);
    dl->AddRect      (pos, pmax, ColU(MvStyle::MvLine2), 4.0f, 0, 1.0f);
    if (MvStyle::Font.MonoSm) {
        ImFont* f = MvStyle::Font.MonoSm;
        constexpr float Sz = 10.0f;
        const ImVec2 ts = f->CalcTextSizeA(Sz, FLT_MAX, 0.0f, text);
        dl->AddText(f, Sz,
                    ImVec2(pos.x + (Size - ts.x) * 0.5f,
                           pos.y + (Size - ts.y) * 0.5f),
                    ColU(MvStyle::MvBlue400), text);
    }
}

} // namespace

Result Render(State state, double tickSeconds) {
    Result out;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::SetNextWindowViewport(vp->ID);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize  |
        ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoDocking   |
        ImGuiWindowFlags_NoBringToFrontOnFocus; // we'll bring to front explicitly

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, MvStyle::MvBg0);

    // SetNextWindowFocus the first frame so input lands here even though
    // the host shell is rendered after us in the loop.
    ImGui::SetNextWindowFocus();
    ImGui::Begin("##MvLoginScreen", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 wpos = ImGui::GetWindowPos();
    const ImVec2 wsize = ImGui::GetWindowSize();
    const ImVec2 wmax(wpos.x + wsize.x, wpos.y + wsize.y);

    // ---- Background: blueprint grid + corner brackets ----------------
    DrawBlueprintGrid(dl, wpos, wmax);
    constexpr float CornerInset = 60.0f;
    DrawCornerBracket(dl, ImVec2(wpos.x + CornerInset, wpos.y + CornerInset),
                      /*flipH=*/false, /*flipV=*/false);
    DrawCornerBracket(dl, ImVec2(wmax.x - CornerInset, wpos.y + CornerInset),
                      /*flipH=*/true,  /*flipV=*/false);
    DrawCornerBracket(dl, ImVec2(wpos.x + CornerInset, wmax.y - CornerInset),
                      /*flipH=*/false, /*flipV=*/true);
    DrawCornerBracket(dl, ImVec2(wmax.x - CornerInset, wmax.y - CornerInset),
                      /*flipH=*/true,  /*flipV=*/true);

    // Esc behaviour:
    //   Waiting → revert to Idle (cancel the in-flight auth flow)
    //   Idle    → dev close (escape hatch only — once auth backend ships
    //             in Sprint 6.1, Esc on Idle will be a no-op since users
    //             must complete sign-in at least once per session).
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        if (state == State::Waiting) out.cancelRequested = true;
        else                          out.closeRequested = true;
    }

    // ---- Auth card ---------------------------------------------------
    constexpr float CardW = 480.0f;
    const float CardH = (state == State::Idle) ? 460.0f : 480.0f;
    const ImVec2 cardCenter(wpos.x + wsize.x * 0.5f,
                            wpos.y + wsize.y * 0.5f);
    const ImVec2 cardMin(cardCenter.x - CardW * 0.5f,
                         cardCenter.y - CardH * 0.5f);
    const ImVec2 cardMax(cardMin.x + CardW, cardMin.y + CardH);

    // Drop shadow — soft offset rect under the card.
    dl->AddRectFilled(ImVec2(cardMin.x + 0.0f, cardMin.y + 8.0f),
                      ImVec2(cardMax.x, cardMax.y + 8.0f),
                      IM_COL32(0, 0, 0, 80), 8.0f);

    // Card surface
    dl->AddRectFilled(cardMin, cardMax, ColU(MvStyle::MvBg2), 8.0f);
    dl->AddRect      (cardMin, cardMax, ColU(MvStyle::MvLine3), 8.0f, 0, 1.0f);

    // 2 px gradient accent bar across the top — transparent → MvBlue500 →
    // MvBlue300 → MvBlue500 → transparent, drawn as 4 rect segments so the
    // colour stops land at predictable positions.
    {
        const float top = cardMin.y;
        const float bot = top + 2.0f;
        const float quarters[5] = {
            cardMin.x,
            cardMin.x + CardW * 0.20f,
            cardMin.x + CardW * 0.50f,
            cardMin.x + CardW * 0.80f,
            cardMax.x,
        };
        const ImU32 c0 = IM_COL32(0, 0, 0, 0);
        const ImU32 c1 = ColU(MvStyle::MvBlue500);
        const ImU32 c2 = ColU(MvStyle::MvBlue300);
        dl->AddRectFilledMultiColor(ImVec2(quarters[0], top),
                                    ImVec2(quarters[1], bot),
                                    c0, c1, c1, c0);
        dl->AddRectFilledMultiColor(ImVec2(quarters[1], top),
                                    ImVec2(quarters[2], bot),
                                    c1, c2, c2, c1);
        dl->AddRectFilledMultiColor(ImVec2(quarters[2], top),
                                    ImVec2(quarters[3], bot),
                                    c2, c1, c1, c2);
        dl->AddRectFilledMultiColor(ImVec2(quarters[3], top),
                                    ImVec2(quarters[4], bot),
                                    c1, c0, c0, c1);
    }

    // ---- Card hero (mark + eyebrow + heading + description) --------
    constexpr float HeroPadL = 32.0f;
    constexpr float HeroPadT = 30.0f;
    constexpr float MarkSize = 64.0f;
    const ImVec2 mMin(cardMin.x + HeroPadL, cardMin.y + HeroPadT);
    const ImVec2 mMax(mMin.x + MarkSize, mMin.y + MarkSize);

    // Same dark-navy backdrop as the welcome modal so the blue logo PNG
    // stands out (welcome's earlier blue gradient hid the silhouette).
    dl->AddRectFilledMultiColor(mMin, mMax,
        ColU(MvStyle::MvBg3), ColU(MvStyle::MvBg3),
        ColU(MvStyle::MvBg4), ColU(MvStyle::MvBg4));
    dl->AddRect(mMin, mMax, ColU(MvStyle::MvBlue400), 10.0f, 0, 1.0f);
    if (MvStyle::Asset.LogoTextureId) {
        constexpr float LogoSize = 42.0f;
        const ImVec2 logoMin((mMin.x + mMax.x - LogoSize) * 0.5f,
                             (mMin.y + mMax.y - LogoSize) * 0.5f);
        const ImVec2 logoMax(logoMin.x + LogoSize, logoMin.y + LogoSize);
        dl->AddImage((ImTextureID)(intptr_t)MvStyle::Asset.LogoTextureId,
                     logoMin, logoMax);
    }

    // Spinner ring around the mark when waiting.
    if (state == State::Waiting) {
        const ImVec2 markCenter((mMin.x + mMax.x) * 0.5f,
                                (mMin.y + mMax.y) * 0.5f);
        DrawSpinnerRing(dl, markCenter, MarkSize * 0.55f + 6.0f, tickSeconds);
    }

    // Hero text block.
    const float textX = mMax.x + 18.0f;
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        dl->AddText(ImVec2(textX, mMin.y + 4.0f),
                    ColU(MvStyle::MvBlue400),
                    state == State::Idle
                        ? "V 2.0.0 \xC2\xB7 SIGN IN REQUIRED"
                        : "V 2.0.0 \xC2\xB7 AWAITING BROWSER");
    }
    {
        MvStyle::ScopedFont f(MvStyle::Font.Display);
        const char* prefix = (state == State::Idle) ? "Sign in to "
                                                     : "Awaiting ";
        const char* accent = (state == State::Idle) ? "MetaVisage"
                                                     : "browser";
        const ImVec2 ts1 = ImGui::CalcTextSize(prefix);
        dl->AddText(ImVec2(textX, mMin.y + 18.0f),
                    ColU(MvStyle::MvFg1), prefix);
        dl->AddText(ImVec2(textX + ts1.x, mMin.y + 18.0f),
                    ColU(MvStyle::MvBlue400), accent);
    }
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        const char* desc = (state == State::Idle)
            ? "Authenticate in your browser to unlock morphing,\nthe asset library, and licensed exports."
            : "We've opened your default browser. Finish there\nand you'll be redirected back automatically.";
        // BodySm is small enough that two-line wrapping is fine without
        // ImGui::TextWrapped — newlines render naturally with AddText.
        dl->AddText(ImVec2(textX, mMin.y + 56.0f),
                    ColU(MvStyle::MvFg2), desc);
    }

    // ---- Step divider rule + label ---------------------------------
    const float divY = cardMin.y + HeroPadT + MarkSize + 36.0f;
    {
        const char* stepLabel = (state == State::Idle)
            ? "STEP 01 / 02 \xE2\x80\x94 AUTHENTICATION"
            : "STEP 02 / 02 \xE2\x80\x94 BROWSER HANDOFF";
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        constexpr float Sz = 10.0f;
        const ImVec2 ts = MvStyle::Font.MonoSm
            ? MvStyle::Font.MonoSm->CalcTextSizeA(Sz, FLT_MAX, 0.0f, stepLabel)
            : ImGui::CalcTextSize(stepLabel);
        const float labelX = (cardMin.x + cardMax.x - ts.x) * 0.5f;
        const float lineGap = 12.0f;
        dl->AddLine(ImVec2(cardMin.x + 36.0f, divY),
                    ImVec2(labelX - lineGap, divY),
                    ColU(MvStyle::MvLine2), 1.0f);
        dl->AddLine(ImVec2(labelX + ts.x + lineGap, divY),
                    ImVec2(cardMax.x - 36.0f, divY),
                    ColU(MvStyle::MvLine2), 1.0f);
        if (MvStyle::Font.MonoSm) {
            dl->AddText(MvStyle::Font.MonoSm, Sz,
                        ImVec2(labelX, divY - Sz * 0.5f),
                        ColU(MvStyle::MvFg3), stepLabel);
        }
    }

    // ---- Body — idle state: button + 3 explainer rows ---------------
    //         waiting state: status block + callback URI + warning
    const float bodyTop = divY + 24.0f;

    if (state == State::Idle) {
        // 48 px-tall full-width primary blue Sign In button.
        const float btnPad = 36.0f;
        const ImVec2 btnMin(cardMin.x + btnPad, bodyTop);
        const float btnW = (cardMax.x - btnPad) - btnMin.x;
        ImGui::SetCursorScreenPos(btnMin);
        ImGui::PushStyleColor(ImGuiCol_Button,        MvStyle::MvBlue500);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, MvStyle::MvBlue400);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  MvStyle::MvBlue600);
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(0x05/255.f, 0x13/255.f, 0x22/255.f, 1.f));
        if (ImGui::Button("Sign in with browser", ImVec2(btnW, 48.0f))) {
            out.signInRequested = true;
        }
        ImGui::PopStyleColor(4);

        // Three explainer rows — square mono badge + body line.
        struct StepRow { const char* num; const char* text; };
        const StepRow rows[3] = {
            { "01", "Your browser opens a secure MetaVisage sign-in page." },
            { "02", "After you authenticate, the browser redirects back." },
            { "03", "The desktop app picks up the session — no copy-paste." },
        };
        const float rowsTop = bodyTop + 48.0f + 24.0f;
        for (int i = 0; i < 3; ++i) {
            const float y = rowsTop + i * 32.0f;
            DrawStepBadge(dl, ImVec2(cardMin.x + 36.0f, y), rows[i].num);
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            dl->AddText(ImVec2(cardMin.x + 36.0f + 32.0f, y + 6.0f),
                        ColU(MvStyle::MvFg2), rows[i].text);
        }
    } else {
        // Waiting state — status block.
        const float pad = 36.0f;
        const ImVec2 boxMin(cardMin.x + pad, bodyTop);
        const ImVec2 boxMax(cardMax.x - pad, bodyTop + 56.0f);
        dl->AddRectFilled(boxMin, boxMax, ColU(MvStyle::MvBg1), 6.0f);
        dl->AddRect      (boxMin, boxMax, ColU(MvStyle::MvLine2), 6.0f, 0, 1.0f);

        // Listener address line + LIVE badge on the right.
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            dl->AddText(ImVec2(boxMin.x + 14.0f, boxMin.y + 10.0f),
                        ColU(MvStyle::MvFg1),
                        "Listening on ");
            const ImVec2 prefixSz = ImGui::CalcTextSize("Listening on ");
            MvStyle::ScopedFont fm(MvStyle::Font.Mono);
            dl->AddText(ImVec2(boxMin.x + 14.0f + prefixSz.x, boxMin.y + 11.0f),
                        ColU(MvStyle::MvBlue400),
                        "metavisage://auth");
        }
        {
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            dl->AddText(ImVec2(boxMin.x + 14.0f, boxMin.y + 32.0f),
                        ColU(MvStyle::MvFg3),
                        "Times out in 2:43");
        }
        {
            // Green LIVE badge on right side of the box.
            constexpr float BadgeW = 56.0f;
            constexpr float BadgeH = 22.0f;
            const ImVec2 badgeMin(boxMax.x - BadgeW - 12.0f,
                                  (boxMin.y + boxMax.y - BadgeH) * 0.5f);
            const ImVec2 badgeMax(badgeMin.x + BadgeW, badgeMin.y + BadgeH);
            dl->AddRectFilled(badgeMin, badgeMax,
                              ColU(WithAlpha(MvStyle::MvSuccess, 0.10f)),
                              4.0f);
            dl->AddRect(badgeMin, badgeMax,
                        ColU(WithAlpha(MvStyle::MvSuccess, 0.45f)),
                        4.0f, 0, 1.0f);
            dl->AddCircleFilled(ImVec2(badgeMin.x + 9.0f,
                                       (badgeMin.y + badgeMax.y) * 0.5f),
                                3.0f, ColU(MvStyle::MvSuccess));
            if (MvStyle::Font.MonoSm) {
                ImFont* f = MvStyle::Font.MonoSm;
                constexpr float Sz = 10.0f;
                const ImVec2 ts = f->CalcTextSizeA(Sz, FLT_MAX, 0.0f, "LIVE");
                dl->AddText(f, Sz,
                            ImVec2(badgeMin.x + 18.0f,
                                   (badgeMin.y + badgeMax.y - ts.y) * 0.5f),
                            ColU(MvStyle::MvSuccess), "LIVE");
            }
        }

        // Callback URI display + Copy button row.
        const float uriY = bodyTop + 70.0f;
        {
            MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
            dl->AddText(ImVec2(cardMin.x + pad, uriY),
                        ColU(MvStyle::MvFg3), "CALLBACK URI");
        }
        {
            const ImVec2 fldMin(cardMin.x + pad, uriY + 18.0f);
            const ImVec2 fldMax(cardMax.x - pad - 70.0f, fldMin.y + 28.0f);
            dl->AddRectFilled(fldMin, fldMax, ColU(MvStyle::MvBg1), 4.0f);
            dl->AddRect      (fldMin, fldMax, ColU(MvStyle::MvLine2), 4.0f, 0, 1.0f);
            MvStyle::ScopedFont f(MvStyle::Font.Mono);
            dl->AddText(ImVec2(fldMin.x + 10.0f,
                               (fldMin.y + fldMax.y) * 0.5f - 6.0f),
                        ColU(MvStyle::MvFg1),
                        "metavisage://auth?session=8f3a\xE2\x80\xA6" "d21c");

            // Copy button — ghost.
            ImGui::SetCursorScreenPos(ImVec2(fldMax.x + 6.0f, fldMin.y));
            if (ImGui::Button("Copy", ImVec2(64.0f, 28.0f))) {
                ImGui::SetClipboardText("metavisage://auth");
            }
        }

        // Amber warning row.
        const float warnY = uriY + 56.0f;
        {
            const ImVec2 wMin(cardMin.x + pad, warnY);
            const ImVec2 wMax(cardMax.x - pad, warnY + 36.0f);
            dl->AddRectFilled(wMin, wMax,
                              ColU(WithAlpha(MvStyle::MvWarning, 0.06f)),
                              6.0f);
            dl->AddRect(wMin, wMax,
                        ColU(WithAlpha(MvStyle::MvWarning, 0.25f)),
                        6.0f, 0, 1.0f);

            // ! badge.
            constexpr float BadgeR = 9.0f;
            const ImVec2 bCenter(wMin.x + 18.0f, (wMin.y + wMax.y) * 0.5f);
            dl->AddCircle(bCenter, BadgeR,
                          ColU(MvStyle::MvWarning), 16, 1.0f);
            if (MvStyle::Font.MonoSm) {
                ImFont* f = MvStyle::Font.MonoSm;
                constexpr float Sz = 10.0f;
                const ImVec2 ts = f->CalcTextSizeA(Sz, FLT_MAX, 0.0f, "!");
                dl->AddText(f, Sz,
                            ImVec2(bCenter.x - ts.x * 0.5f,
                                   bCenter.y - ts.y * 0.5f),
                            ColU(MvStyle::MvWarning), "!");
            }

            // Warning text.
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            dl->AddText(ImVec2(wMin.x + 36.0f, wMin.y + 11.0f),
                        ColU(MvStyle::MvFg1),
                        "Browser didn't open?");
            ImGui::SetCursorScreenPos(ImVec2(wMin.x + 36.0f + 134.0f,
                                             wMin.y + 9.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  WithAlpha(MvStyle::MvBlue500, 0.10f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  WithAlpha(MvStyle::MvBlue500, 0.20f));
            ImGui::PushStyleColor(ImGuiCol_Text, MvStyle::MvBlue400);
            if (ImGui::Button("Retry", ImVec2(56.0f, 20.0f))) {
                out.retryRequested = true;
            }
            ImGui::PopStyleColor(4);
        }
    }

    // ---- Card footer: SSO label + ghost button -----------------------
    constexpr float FooterH = 50.0f;
    const ImVec2 fMin(cardMin.x, cardMax.y - FooterH);
    const ImVec2 fMax(cardMax.x, cardMax.y);
    dl->AddRectFilled(fMin, fMax, ColU(MvStyle::MvBg1));
    dl->AddLine(ImVec2(fMin.x, fMin.y),
                ImVec2(fMax.x, fMin.y),
                ColU(MvStyle::MvLine2), 1.0f);

    // Lock glyph + "SSO · OAuth 2.1 · PKCE" mono on the left.
    {
        constexpr float pad = 36.0f;
        if (MvStyle::Font.IconSm) {
            ImFont* f = MvStyle::Font.IconSm;
            constexpr float Sz = 12.0f;
            const ImVec2 ts = f->CalcTextSizeA(Sz, FLT_MAX, 0.0f, MV_ICON_LOCK);
            dl->AddText(f, Sz,
                        ImVec2(fMin.x + pad,
                               (fMin.y + fMax.y - ts.y) * 0.5f),
                        ColU(MvStyle::MvFg3), MV_ICON_LOCK);
        }
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        dl->AddText(ImVec2(fMin.x + pad + 18.0f,
                           (fMin.y + fMax.y) * 0.5f - 5.0f),
                    ColU(MvStyle::MvFg3),
                    "SSO \xC2\xB7 OAUTH 2.1 \xC2\xB7 PKCE");
    }

    // Right-side ghost button — Cancel only when waiting on the browser.
    // Idle state has no escape: the design's "Use offline" option is
    // omitted by product policy (users must complete sign-in at least
    // once per session), so the right side of the footer stays empty
    // until auth is in flight.
    if (state == State::Waiting) {
        constexpr float pad = 36.0f;
        constexpr float BtnW = 96.0f;
        ImGui::SetCursorScreenPos(ImVec2(fMax.x - pad - BtnW,
                                         (fMin.y + fMax.y) * 0.5f - 14.0f));
        if (ImGui::Button("Cancel", ImVec2(BtnW, 28.0f))) {
            out.cancelRequested = true;
        }
    }

    // ---- Bottom metadata strip --------------------------------------
    {
        const char* metaParts[] = {
            "BUILD 2.0.0-rc.1",
            "NODE auth-eu-2",
            "HELP",
            "PRIVACY",
        };
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        constexpr float Sz = 10.0f;
        constexpr float Sep = 18.0f;
        float totalW = 0.0f;
        float partW[4] = {0};
        for (int i = 0; i < 4; ++i) {
            const ImVec2 ts = MvStyle::Font.MonoSm
                ? MvStyle::Font.MonoSm->CalcTextSizeA(Sz, FLT_MAX, 0.0f, metaParts[i])
                : ImGui::CalcTextSize(metaParts[i]);
            partW[i] = ts.x;
            totalW += ts.x;
            if (i < 3) totalW += Sep;
        }
        const float startX = wpos.x + (wsize.x - totalW) * 0.5f;
        const float yMeta  = wmax.y - 26.0f;
        float cx = startX;
        for (int i = 0; i < 4; ++i) {
            ImVec4 col = (i >= 2) ? MvStyle::MvFg3 : MvStyle::MvFg4;
            if (MvStyle::Font.MonoSm) {
                dl->AddText(MvStyle::Font.MonoSm, Sz,
                            ImVec2(cx, yMeta), ColU(col), metaParts[i]);
            }
            cx += partW[i];
            if (i < 3) {
                dl->AddText(ImVec2(cx + Sep * 0.5f - 2.0f, yMeta),
                            ColU(MvStyle::MvLine3), "\xC2\xB7");
                cx += Sep;
            }
        }
    }

    // ---- Top-right dev close X (escape hatch while no auth backend) -
    // Once Sprint 6.1's auth flow lands, this disappears and the only
    // way out is Use offline / successful sign-in. Keeping it here lets
    // the dev review the screen without locking themselves out of the
    // app shell.
    {
        constexpr float CloseW = 28.0f;
        const ImVec2 closeMin(wmax.x - CloseW - 14.0f, wpos.y + 14.0f);
        ImGui::SetCursorScreenPos(closeMin);
        if (ImGui::InvisibleButton("##loginClose",
                                   ImVec2(CloseW, CloseW))) {
            out.closeRequested = true;
        }
        const bool hovered = ImGui::IsItemHovered();
        const ImU32 closeCol = hovered
            ? ColU(MvStyle::MvFg1) : ColU(MvStyle::MvFg3);
        if (MvStyle::Font.IconSm) {
            ImFont* f = MvStyle::Font.IconSm;
            constexpr float Sz = 14.0f;
            const ImVec2 ts = f->CalcTextSizeA(Sz, FLT_MAX, 0.0f, MV_ICON_X);
            dl->AddText(f, Sz,
                        ImVec2(closeMin.x + (CloseW - ts.x) * 0.5f,
                               closeMin.y + (CloseW - ts.y) * 0.5f),
                        closeCol, MV_ICON_X);
        }
    }

    ImGui::End();
    return out;
}

} // namespace MetaVisage::LoginScreen
