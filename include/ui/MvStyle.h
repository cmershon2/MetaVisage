#ifndef METAVISAGE_UI_MVSTYLE_H
#define METAVISAGE_UI_MVSTYLE_H

#include <imgui.h>

// Sprint 3.8 design system — direct mirror of
// .agents/context/design/version_2/colors_and_type.css.
//
// All UI surfaces should source colors from this header. Never write a raw
// ImVec4(r,g,b,a) literal in stage / modal / overlay code — if a token is
// missing here, add it here first.

namespace MetaVisage::MvStyle {

// =============================================================================
// Color tokens
// =============================================================================

// Surface (dark navy)
inline const ImVec4 MvBg0 = ImVec4(0x07/255.f, 0x10/255.f, 0x1C/255.f, 1.f); // page void
inline const ImVec4 MvBg1 = ImVec4(0x0B/255.f, 0x16/255.f, 0x22/255.f, 1.f); // primary canvas
inline const ImVec4 MvBg2 = ImVec4(0x10/255.f, 0x1F/255.f, 0x30/255.f, 1.f); // card / panel
inline const ImVec4 MvBg3 = ImVec4(0x16/255.f, 0x28/255.f, 0x3C/255.f, 1.f); // raised / hover
inline const ImVec4 MvBg4 = ImVec4(0x1E/255.f, 0x35/255.f, 0x50/255.f, 1.f); // selected row

// Brand blue (blueprint)
inline const ImVec4 MvBlue300 = ImVec4(0x7B/255.f, 0xC2/255.f, 0xFA/255.f, 1.f); // highlight / hover
inline const ImVec4 MvBlue400 = ImVec4(0x4D/255.f, 0xAE/255.f, 0xF7/255.f, 1.f); // secondary brand
inline const ImVec4 MvBlue500 = ImVec4(0x3D/255.f, 0xA5/255.f, 0xF5/255.f, 1.f); // PRIMARY accent
inline const ImVec4 MvBlue600 = ImVec4(0x1E/255.f, 0x8F/255.f, 0xE0/255.f, 1.f); // pressed / active
inline const ImVec4 MvBlue700 = ImVec4(0x15/255.f, 0x72/255.f, 0xB8/255.f, 1.f); // outlined accent
inline const ImVec4 MvBlue900 = ImVec4(0x0E/255.f, 0x3F/255.f, 0x66/255.f, 1.f); // deep glow

// Foreground
inline const ImVec4 MvFg1 = ImVec4(0xE8/255.f, 0xF1/255.f, 0xFB/255.f, 1.f); // primary text
inline const ImVec4 MvFg2 = ImVec4(0xA8/255.f, 0xBB/255.f, 0xD0/255.f, 1.f); // secondary text
inline const ImVec4 MvFg3 = ImVec4(0x6F/255.f, 0x83/255.f, 0x9A/255.f, 1.f); // tertiary / hint
inline const ImVec4 MvFg4 = ImVec4(0x43/255.f, 0x57/255.f, 0x6E/255.f, 1.f); // disabled / placeholder

// Lines / blueprint grid (alpha-blended over MvBg1)
inline const ImVec4 MvLine1 = ImVec4(125/255.f, 175/255.f, 220/255.f, 0.08f); // hairline / 1px grid
inline const ImVec4 MvLine2 = ImVec4(125/255.f, 175/255.f, 220/255.f, 0.16f); // divider
inline const ImVec4 MvLine3 = ImVec4(125/255.f, 175/255.f, 220/255.f, 0.32f); // prominent divider
inline const ImVec4 MvLine4 = ImVec4(125/255.f, 175/255.f, 220/255.f, 0.55f); // wireframe stroke

// Semantic
inline const ImVec4 MvSuccess = ImVec4(0x3F/255.f, 0xD6/255.f, 0x9E/255.f, 1.f);
inline const ImVec4 MvWarning = ImVec4(0xF2/255.f, 0xB5/255.f, 0x47/255.f, 1.f);
inline const ImVec4 MvDanger  = ImVec4(0xF2/255.f, 0x6A/255.f, 0x6A/255.f, 1.f);

// Axis colors used by Stage 1 transform inputs (X/Y/Z numeric fields)
inline const ImVec4 MvAxisX = ImVec4(0xFF/255.f, 0x6B/255.f, 0x6B/255.f, 1.f);
inline const ImVec4 MvAxisY = ImVec4(0x3F/255.f, 0xD6/255.f, 0x9E/255.f, 1.f);
inline const ImVec4 MvAxisZ = ImVec4(0x3D/255.f, 0xA5/255.f, 0xF5/255.f, 1.f);

// =============================================================================
// Spacing scale (4px base) — matches CSS --mv-s-N
// =============================================================================
namespace Spacing {
    inline constexpr float S1 = 4.0f;
    inline constexpr float S2 = 8.0f;
    inline constexpr float S3 = 12.0f;
    inline constexpr float S4 = 16.0f;
    inline constexpr float S5 = 24.0f;
    inline constexpr float S6 = 32.0f;
    inline constexpr float S7 = 48.0f;
    inline constexpr float S8 = 64.0f;
    inline constexpr float S9 = 96.0f;
}

// Sprint 10.4 — menu popups get roomier rows than the global 6 px
// ItemSpacing.y, which read as crowded in dropdowns.
namespace Menu {
    inline constexpr ImVec2 WindowPadding = ImVec2(Spacing::S2, Spacing::S2);
    inline constexpr ImVec2 ItemSpacing   = ImVec2(Spacing::S4, Spacing::S3);
}

// =============================================================================
// Type scale — concrete font-pixel sizes used at AddFontFromFileTTF time.
// Each handle in `Font` below corresponds to one entry here.
// =============================================================================
namespace TypeSize {
    inline constexpr float DisplayLg = 32.0f;  // welcome / login hero
    inline constexpr float Display   = 28.0f;  // stage header
    inline constexpr float Heading   = 20.0f;  // section H4
    inline constexpr float BodyLg    = 16.0f;  // stepper stage titles, prominent labels
    inline constexpr float Body      = 14.0f;  // labels, buttons, body
    inline constexpr float BodySm    = 13.0f;  // hint text, descriptions
    inline constexpr float MonoMd    = 13.0f;  // stepper stage numbers (disc-sized mono)
    inline constexpr float Mono      = 11.0f;  // technical readouts, hotkey strips
    inline constexpr float MonoSm    = 10.0f;  // section labels, eyebrows
}

// =============================================================================
// Font handles — populated by LoadFonts(). Null until loaded; renderers must
// guard ImGui::PushFont calls with `if (MvStyle::Font.Body)` because the
// app must still launch when the TTFs are missing from assets/fonts/.
// =============================================================================
struct FontHandles {
    ImFont* DisplayLg = nullptr;  // Saira ExtraBoldItalic        32 px
    ImFont* Display   = nullptr;  // Saira ExtraBoldItalic        28 px
    ImFont* Heading   = nullptr;  // Inter_24pt SemiBold           20 px
    ImFont* BodyLg    = nullptr;  // Inter_18pt Medium             16 px
    ImFont* Body      = nullptr;  // Inter_18pt Regular            14 px  (also io.FontDefault)
    ImFont* BodyMd    = nullptr;  // Inter_18pt Medium             14 px
    ImFont* BodySm    = nullptr;  // Inter_18pt Regular            13 px
    ImFont* MonoMd    = nullptr;  // JetBrainsMono Medium          13 px
    ImFont* Mono      = nullptr;  // JetBrainsMono Regular         11 px
    ImFont* MonoSm    = nullptr;  // JetBrainsMono Medium          10 px
    ImFont* Icon      = nullptr;  // lucide.ttf                    16 px  (Story 3.8.5)
    ImFont* IconSm    = nullptr;  // lucide.ttf                    13 px
};
extern FontHandles Font;

// =============================================================================
// Image / GL texture assets — populated by LoadAssets().
// =============================================================================
struct AssetHandles {
    unsigned int LogoTextureId = 0;  // metavisage-logo-mark.png — title bar / welcome / login
    int          LogoWidth     = 0;
    int          LogoHeight    = 0;

    // Story 5.3 follow-up — Stage 4 brush preview textures. Index order
    // matches kBrushes in StageSidebar.cpp::RenderTouchUp:
    //   0 Smooth, 1 Grab, 2 PushPull, 3 Inflate.
    // Zero means "load failed" — the brush card silently falls back to
    // the Lucide glyph in that case so the panel stays usable.
    unsigned int BrushPreviewTextureIds[4] = { 0, 0, 0, 0 };

};
extern AssetHandles Asset;

// =============================================================================
// Lifecycle — call sequence in AppWindow::Initialize:
//
//   ImGui::CreateContext();
//   MvStyle::LoadFonts();          // builds the font atlas
//   MvStyle::Apply();              // sets ImGui::GetStyle() colors / sizes
//   ImGui_ImplGlfw_InitForOpenGL(...);
//   ImGui_ImplOpenGL3_Init(...);   // uploads the atlas to the GPU
//   MvStyle::LoadAssets();         // PNGs that need a GL context
//
// And in AppWindow::Shutdown, before ImGui::DestroyContext:
//
//   MvStyle::Shutdown();
// =============================================================================

void Apply();         // Idempotent ImGuiStyle setup. Safe to call multiple times.
void LoadFonts();     // After ImGui::CreateContext, before ImGui_ImplOpenGL3_Init.
void LoadAssets();    // After ImGui_ImplOpenGL3_Init (needs GL context).
void Shutdown();      // Free GL textures. Call before ImGui::DestroyContext.

// =============================================================================
// ScopedFont — RAII helper that PushFonts on construction and PopFonts on
// destruction, but only if the supplied handle is non-null. Lets call sites
// stay clean even when the design fonts haven't been bundled yet (LoadFonts
// leaves missing handles null and falls back to ProggyClean).
//
//   { MvStyle::ScopedFont f(MvStyle::Font.Mono);
//     ImGui::Text("11px JetBrainsMono Regular text"); }
// =============================================================================
class ScopedFont {
public:
    explicit ScopedFont(ImFont* f) : pushed_(f != nullptr) {
        if (pushed_) ImGui::PushFont(f);
    }
    ~ScopedFont() { if (pushed_) ImGui::PopFont(); }
    ScopedFont(const ScopedFont&) = delete;
    ScopedFont& operator=(const ScopedFont&) = delete;
private:
    bool pushed_;
};

} // namespace MetaVisage::MvStyle

#endif // METAVISAGE_UI_MVSTYLE_H
