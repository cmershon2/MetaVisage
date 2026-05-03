#include "ui/LoginScreen.h"

#include "ui/MvIcons.h"
#include "ui/MvStyle.h"

#include <imgui.h>

#include <cstdio>

namespace MetaVisage::LoginScreen {

namespace {

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

// PR #25 review feedback removed the Waiting-state spinner ring around
// the logo mark. DrawSpinnerRing was its only caller, so the function
// is gone too -- restore from git history if a future design adds a
// spinner back to the auth card.

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

Result Render(State state) {
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

    // No SetNextWindowFocus — when the title bar is rendered AFTER us in
    // login mode (so its caption buttons sit on top), pinning focus to
    // the login window every frame would route every click to login and
    // swallow caption-button hits. ImGui's default hit-testing finds the
    // topmost interactive item under the cursor regardless of focus, so
    // dropping the explicit focus call lets clicks land on whichever
    // window is actually drawn on top. Login still receives input on its
    // own buttons because it's the only other interactive window in this
    // mode (host shell is suppressed).
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

    // Esc behaviour (Sprint 6.1):
    //   Waiting -> revert to Idle (cancel the in-flight auth flow)
    //   Idle    -> no-op (sign-in is mandatory; the dev close-on-Esc path
    //              was scaffolding from Sprint 3.8.12 and is gone)
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && state == State::Waiting) {
        out.cancelRequested = true;
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

    // PR #25 review feedback: removed the spinner ring around the logo
    // mark in Waiting state; the design team felt it read as broken
    // around the square logo. The "AWAITING BROWSER" caption + hero
    // text below already communicate the in-flight state.

    // Hero text block.
    const float textX = mMax.x + 18.0f;
    {
        MvStyle::ScopedFont f(MvStyle::Font.MonoSm);
        const char* caption = "V 2.0.0 \xC2\xB7 SIGN IN REQUIRED";
        if (state == State::Waiting)   caption = "V 2.0.0 \xC2\xB7 AWAITING BROWSER";
        if (state == State::Verifying) caption = "V 2.0.0 \xC2\xB7 VERIFYING SESSION";
        dl->AddText(ImVec2(textX, mMin.y + 4.0f),
                    ColU(MvStyle::MvBlue400), caption);
    }
    {
        MvStyle::ScopedFont f(MvStyle::Font.Display);
        const char* prefix = "Sign in to ";
        const char* accent = "MetaVisage";
        if (state == State::Waiting)   { prefix = "Awaiting ";   accent = "browser"; }
        if (state == State::Verifying) { prefix = "Verifying ";  accent = "session"; }
        const ImVec2 ts1 = ImGui::CalcTextSize(prefix);
        dl->AddText(ImVec2(textX, mMin.y + 18.0f),
                    ColU(MvStyle::MvFg1), prefix);
        dl->AddText(ImVec2(textX + ts1.x, mMin.y + 18.0f),
                    ColU(MvStyle::MvBlue400), accent);
    }
    {
        MvStyle::ScopedFont f(MvStyle::Font.BodySm);
        const char* desc = "Authenticate in your browser to unlock morphing,\nthe asset library, and licensed exports.";
        if (state == State::Waiting) {
            desc = "We've opened your default browser. Finish there\nand you'll be redirected back automatically.";
        } else if (state == State::Verifying) {
            desc = "Securing your session and fetching license details.\nThis usually takes a second.";
        }
        // BodySm is small enough that two-line wrapping is fine without
        // ImGui::TextWrapped — newlines render naturally with AddText.
        dl->AddText(ImVec2(textX, mMin.y + 56.0f),
                    ColU(MvStyle::MvFg2), desc);
    }

    // ---- Step divider rule + label ---------------------------------
    const float divY = cardMin.y + HeroPadT + MarkSize + 36.0f;
    {
        const char* stepLabel = "STEP 01 / 02 \xE2\x80\x94 AUTHENTICATION";
        if (state == State::Waiting)   stepLabel = "STEP 02 / 02 \xE2\x80\x94 BROWSER HANDOFF";
        if (state == State::Verifying) stepLabel = "STEP 02 / 02 \xE2\x80\x94 SESSION HANDOFF";
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
    } else if (state == State::Verifying) {
        // Verifying state body (PR #25 UI feedback): deep link arrived,
        // exchange POST is in flight on a worker thread. Render a
        // dedicated loading view so the user knows the app is working
        // and so they can't trigger a duplicate Sign In via the
        // (now-hidden) Retry button. Cancel stays in the footer in
        // case the exchange hangs.
        const float pad = 36.0f;
        // Animated dots: 0 -> 3 dots, ~1.5 Hz. Tracks ImGui::GetTime()
        // so the indicator updates at frame cadence with no extra
        // plumbing. Buffer is "Verifying your session..." with the
        // trailing 3 chars rewritten each frame.
        char buf[40];
        const int phase =
            static_cast<int>(ImGui::GetTime() * 1.5) & 3;  // 0..3
        std::snprintf(buf, sizeof(buf),
                      "Verifying your session%c%c%c",
                      (phase > 0) ? '.' : ' ',
                      (phase > 1) ? '.' : ' ',
                      (phase > 2) ? '.' : ' ');
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            dl->AddText(ImVec2(cardMin.x + pad, bodyTop + 4.0f),
                        ColU(MvStyle::MvFg1), buf);
            dl->AddText(ImVec2(cardMin.x + pad, bodyTop + 26.0f),
                        ColU(MvStyle::MvFg3),
                        "Securely exchanging your sign-in for a license token.");
        }
        // No retry warning row -- a click here would cancel the
        // in-flight exchange and re-launch the browser, which is
        // exactly the wrong behavior. Cancel button in the footer
        // remains the only escape hatch.
    } else {
        // Waiting state body (PR #25 review feedback): the listening
        // address box, callback URI display, Copy button, and LIVE
        // badge were design scaffolding that surfaced internal scheme
        // detail. Replaced with a single explanatory paragraph that
        // tells the user where to look. The amber "Browser didn't
        // open? Retry" row stays since it's the recovery affordance.
        const float pad = 36.0f;
        {
            MvStyle::ScopedFont f(MvStyle::Font.BodySm);
            dl->AddText(ImVec2(cardMin.x + pad, bodyTop + 4.0f),
                        ColU(MvStyle::MvFg1),
                        "We\xE2\x80\x99ve opened your default browser.");
            dl->AddText(ImVec2(cardMin.x + pad, bodyTop + 26.0f),
                        ColU(MvStyle::MvFg3),
                        "Complete sign-in there and you\xE2\x80\x99ll be returned to the desktop app automatically.");
        }

        // Amber warning row.
        const float warnY = bodyTop + 80.0f;
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

    // Right-side ghost button — Cancel during browser-wait or during
    // the post-deep-link exchange. Idle state has no escape (the
    // design's "Use offline" option is omitted by product policy --
    // users must complete sign-in at least once per session), so the
    // right side of the footer stays empty until auth is in flight.
    if (state == State::Waiting || state == State::Verifying) {
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

    ImGui::End();
    return out;
}

} // namespace MetaVisage::LoginScreen
