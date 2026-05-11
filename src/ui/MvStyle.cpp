#include "ui/MvStyle.h"

#include "utils/Logger.h"
#include "utils/Paths.h"

// STB_IMAGE_IMPLEMENTATION is provided by src/core/TextureData.cpp; here we
// only need the public stbi_load / stbi_image_free / stbi_failure_reason API.
#include "thirdparty/stb_image.h"

#include <GL/glew.h>

#include <filesystem>
#include <format>
#include <iterator>
#include <string>

namespace MetaVisage::MvStyle {

FontHandles  Font;
AssetHandles Asset;

namespace {

ImVec4 WithAlpha(const ImVec4& c, float a) {
    return ImVec4(c.x, c.y, c.z, a);
}

// Try to load a TTF font. Returns nullptr if the file is missing or the
// atlas rejects it. Both failure paths log the resolved path so a Story 3.8.1
// install sanity check is one grep away in the log.
ImFont* TryLoadFont(ImFontAtlas& atlas, const std::filesystem::path& path, float pxSize) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        MV_LOG_WARNING(std::format("MvStyle: font missing, will use default fallback: {}",
                                   path.string()));
        return nullptr;
    }
    ImFont* f = atlas.AddFontFromFileTTF(path.string().c_str(), pxSize);
    if (!f) {
        MV_LOG_WARNING(std::format("MvStyle: AddFontFromFileTTF failed for {} ({} px)",
                                   path.string(), pxSize));
    }
    return f;
}

} // namespace

void Apply() {
    // Start from StyleColorsDark so any ImGuiCol slot we don't explicitly
    // override falls back to a sensible dark default rather than the bright
    // ImGui Classic palette.
    ImGui::StyleColorsDark();

    ImGuiStyle& s = ImGui::GetStyle();

    // Rounding — picks the CSS --mv-r-* values that map to a distinct ImGui
    // slot (CSS has more radius variants than ImGui exposes).
    s.WindowRounding    = 6.0f;
    s.ChildRounding     = 6.0f;
    s.FrameRounding     = 4.0f;
    s.PopupRounding     = 8.0f;
    s.ScrollbarRounding = 6.0f;
    s.GrabRounding      = 4.0f;
    s.TabRounding       = 4.0f;

    // 1px hairline borders everywhere — matches the design's blueprint feel.
    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize  = 1.0f;
    s.PopupBorderSize  = 1.0f;
    s.FrameBorderSize  = 1.0f;
    s.TabBorderSize    = 1.0f;

    // Spacing — 4px base scale identical to CSS --mv-s-N.
    s.WindowPadding    = ImVec2(Spacing::S4, Spacing::S4);
    s.FramePadding     = ImVec2(Spacing::S3, 6.0f);
    s.ItemSpacing      = ImVec2(Spacing::S2, 6.0f);
    s.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    s.IndentSpacing    = Spacing::S5;
    s.ScrollbarSize    = 10.0f;
    s.GrabMinSize      = 10.0f;

    auto& C = s.Colors;

    // Surfaces
    C[ImGuiCol_WindowBg]            = MvBg1;
    C[ImGuiCol_ChildBg]             = MvBg1;
    C[ImGuiCol_PopupBg]             = MvBg2;
    C[ImGuiCol_Border]              = MvLine2;
    C[ImGuiCol_BorderShadow]        = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Frames (inputs, sliders, drags)
    C[ImGuiCol_FrameBg]             = MvBg1;
    C[ImGuiCol_FrameBgHovered]      = MvBg3;
    C[ImGuiCol_FrameBgActive]       = MvBg4;

    // Title / menu bars
    C[ImGuiCol_TitleBg]             = MvBg0;
    C[ImGuiCol_TitleBgActive]       = MvBg2;
    C[ImGuiCol_TitleBgCollapsed]    = MvBg0;
    C[ImGuiCol_MenuBarBg]           = MvBg2;

    // Scrollbars
    C[ImGuiCol_ScrollbarBg]         = MvBg1;
    C[ImGuiCol_ScrollbarGrab]       = MvBg4;
    C[ImGuiCol_ScrollbarGrabHovered]= MvLine3;
    C[ImGuiCol_ScrollbarGrabActive] = MvLine4;

    // Active accents — primary brand blue
    C[ImGuiCol_CheckMark]           = MvBlue500;
    C[ImGuiCol_SliderGrab]          = MvBlue500;
    C[ImGuiCol_SliderGrabActive]    = MvBlue600;

    // Buttons (default ghost-style; primary blue is applied per-call by the
    // inspector primitives in Story 3.8.6, not via the global style).
    C[ImGuiCol_Button]              = MvBg3;
    C[ImGuiCol_ButtonHovered]       = MvBg4;
    C[ImGuiCol_ButtonActive]        = MvBg2;

    // Headers (CollapsingHeader, Selectable, TreeNode, Selected MenuItem)
    C[ImGuiCol_Header]              = MvBg3;
    C[ImGuiCol_HeaderHovered]       = MvBg4;
    C[ImGuiCol_HeaderActive]        = WithAlpha(MvBlue500, 0.30f);

    // Separators
    C[ImGuiCol_Separator]           = MvLine2;
    C[ImGuiCol_SeparatorHovered]    = MvLine3;
    C[ImGuiCol_SeparatorActive]     = MvLine4;

    // Resize grips
    C[ImGuiCol_ResizeGrip]          = WithAlpha(MvBlue500, 0.20f);
    C[ImGuiCol_ResizeGripHovered]   = WithAlpha(MvBlue500, 0.50f);
    C[ImGuiCol_ResizeGripActive]    = WithAlpha(MvBlue500, 0.85f);

    // Tabs
    C[ImGuiCol_Tab]                 = MvBg2;
    C[ImGuiCol_TabHovered]          = MvBg3;
    C[ImGuiCol_TabActive]           = MvBg4;
    C[ImGuiCol_TabUnfocused]        = MvBg2;
    C[ImGuiCol_TabUnfocusedActive]  = MvBg3;

    // Docking
    C[ImGuiCol_DockingPreview]      = WithAlpha(MvBlue500, 0.30f);
    C[ImGuiCol_DockingEmptyBg]      = MvBg0;

    // Plots (rare in this app, but kept on-palette so debug overlays match)
    C[ImGuiCol_PlotLines]           = MvBlue400;
    C[ImGuiCol_PlotLinesHovered]    = MvBlue300;
    C[ImGuiCol_PlotHistogram]       = MvBlue500;
    C[ImGuiCol_PlotHistogramHovered]= MvBlue400;

    // Selection / drag-drop / nav
    C[ImGuiCol_TextSelectedBg]      = WithAlpha(MvBlue500, 0.35f);
    C[ImGuiCol_DragDropTarget]      = MvBlue500;
    C[ImGuiCol_NavHighlight]        = MvBlue500;

    // Text
    C[ImGuiCol_Text]                = MvFg1;
    C[ImGuiCol_TextDisabled]        = MvFg4;
}

void LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    ImFontAtlas& atlas = *io.Fonts;
    atlas.Clear();

    const std::filesystem::path fontDir =
        std::filesystem::path(Paths::ExecutableDir()) / "assets" / "fonts";

    // Inter — body / labels / buttons. Google Fonts ships Inter v4 as a
    // variable font with static optical-size cuts at 18pt, 24pt, and 28pt.
    // The 18pt cut is hinted for body-size rendering (~13–18 px) which
    // covers BodySm/Body, while the 24pt cut is tighter and designed for
    // the 20–24 px heading range. We use both to keep the typography on-spec.
    // The "_18pt" / "_24pt" suffix is the canonical filename produced by the
    // Google Fonts download flow as of 2026; older Inter v3 downloads
    // (Inter-Regular.ttf, etc.) are not used.
    Font.Body    = TryLoadFont(atlas, fontDir / "Inter_18pt-Regular.ttf",  TypeSize::Body);
    Font.BodySm  = TryLoadFont(atlas, fontDir / "Inter_18pt-Regular.ttf",  TypeSize::BodySm);
    Font.BodyMd  = TryLoadFont(atlas, fontDir / "Inter_18pt-Medium.ttf",   TypeSize::Body);
    Font.Heading = TryLoadFont(atlas, fontDir / "Inter_24pt-SemiBold.ttf", TypeSize::Heading);

    // Saira ExtraBold Italic — bold display headings only. The wordmark is
    // italic; non-italic Saira is not used.
    Font.Display   = TryLoadFont(atlas, fontDir / "Saira-ExtraBoldItalic.ttf", TypeSize::Display);
    Font.DisplayLg = TryLoadFont(atlas, fontDir / "Saira-ExtraBoldItalic.ttf", TypeSize::DisplayLg);

    // JetBrains Mono — technical readouts (vert counts, hotkey strips,
    // section eyebrows, status bar fields).
    Font.Mono   = TryLoadFont(atlas, fontDir / "JetBrainsMono-Regular.ttf", TypeSize::Mono);
    Font.MonoSm = TryLoadFont(atlas, fontDir / "JetBrainsMono-Medium.ttf",  TypeSize::MonoSm);

    // Story 3.8.5+ — Lucide icon font. Glyphs live in the Unicode Private
    // Use Area (U+E000–U+F8FF). We pass an explicit static glyph range
    // covering that block so ImGui rasterizes only what we'll actually use,
    // keeping the atlas footprint reasonable. The exact icon→codepoint
    // mapping depends on which Lucide font build is bundled (lucide-static
    // ships an info.json; a Fontello build ships config.json) — see
    // include/ui/MvIcons.h for the project's resolved constants.
    static const ImWchar kIconRanges[] = {
        0xE000, 0xF8FF, // PUA — covers all Lucide glyph codepoints
        0,
    };
    {
        const std::filesystem::path lucidePath = fontDir / "lucide.ttf";
        std::error_code ec;
        if (std::filesystem::exists(lucidePath, ec)) {
            ImFontConfig cfg;
            cfg.GlyphRanges = kIconRanges;
            cfg.PixelSnapH = true;
            // The icon glyphs are designed on a 24×24 viewBox — render
            // slightly larger than body text so they read as proper
            // toolbar icons rather than tiny inline glyphs.
            Font.Icon   = atlas.AddFontFromFileTTF(lucidePath.string().c_str(), 16.0f, &cfg);
            Font.IconSm = atlas.AddFontFromFileTTF(lucidePath.string().c_str(), 13.0f, &cfg);
            if (!Font.Icon) {
                MV_LOG_WARNING(std::format("MvStyle: AddFontFromFileTTF failed for {}",
                                           lucidePath.string()));
            }
        } else {
            MV_LOG_WARNING(std::format(
                "MvStyle: lucide.ttf not found at {} — icons will fall back to "
                "ImDrawList primitives where available, missing-glyph rects elsewhere",
                lucidePath.string()));
        }
    }

    // Promote Inter Regular @ body size to ImGui's default font. Any UI we
    // haven't migrated to a named handle still picks up Inter rather than
    // ProggyClean while the per-stage stories land.
    if (Font.Body) {
        io.FontDefault = Font.Body;
    } else {
        // No design fonts loaded — fall back to ProggyClean so the app still
        // launches. Bundling the TTFs into assets/fonts/ is intentionally
        // soft: a dev who pulls the repo without the assets folder still
        // gets a usable build.
        atlas.AddFontDefault();
        MV_LOG_WARNING(
            "MvStyle: no design fonts loaded — falling back to ImGui ProggyClean. "
            "Drop Inter-Regular.ttf, Inter-Medium.ttf, Inter-SemiBold.ttf, "
            "Saira-ExtraBoldItalic.ttf, JetBrainsMono-Regular.ttf, and "
            "JetBrainsMono-Medium.ttf into assets/fonts/ to enable redesign typography.");
    }
}

// Helper: stbi_load + glTexImage2D + free. Returns 0 on failure (caller
// is expected to fall back gracefully). Mip-min/mag = GL_LINEAR with
// edge clamp matches the existing logo pipeline.
namespace {

GLuint UploadPngAsTexture(const std::filesystem::path& path,
                           int* outWidth = nullptr,
                           int* outHeight = nullptr) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        MV_LOG_WARNING(std::format("MvStyle: image not found at {}", path.string()));
        return 0;
    }

    int w = 0, h = 0, sourceChannels = 0;
    unsigned char* pixels =
        stbi_load(path.string().c_str(), &w, &h, &sourceChannels, /*req=*/4);
    if (!pixels) {
        MV_LOG_WARNING(std::format("MvStyle: stbi_load failed for {}: {}",
                                   path.string(), stbi_failure_reason()));
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    stbi_image_free(pixels);

    if (outWidth)  *outWidth  = w;
    if (outHeight) *outHeight = h;
    return tex;
}

} // namespace

void LoadAssets() {
    const std::filesystem::path assetsRoot =
        std::filesystem::path(Paths::ExecutableDir()) / "assets";

    // Brand logo (existing path).
    int logoW = 0, logoH = 0;
    Asset.LogoTextureId =
        UploadPngAsTexture(assetsRoot / "metavisage-logo-mark.png", &logoW, &logoH);
    if (Asset.LogoTextureId) {
        Asset.LogoWidth  = logoW;
        Asset.LogoHeight = logoH;
        MV_LOG_INFO(std::format("MvStyle: logo loaded {}x{} (GL tex {})",
                                logoW, logoH, Asset.LogoTextureId));
    }

    // Story 5.3 — Stage 4 brush preview thumbnails. Filenames are
    // load-bearing: the order here MUST match kBrushes in
    // StageSidebar.cpp::RenderTouchUp (Smooth / Grab / PushPull / Inflate).
    static constexpr const char* kBrushPreviewFilenames[] = {
        "smooth_preview.png",
        "grab_preview.png",
        "pushpull_preview.png",
        "inflate_preview.png",
    };
    // PR #24 review nit — compile-time guard so the filename list and
    // the texture-handle array can't drift in size. The loop below is
    // the single load site; ToolRail.cpp has a sibling assert pinning
    // BrushPreviewTextureIds against kStage4Tools.
    static_assert(std::size(kBrushPreviewFilenames) ==
                      std::size(Asset.BrushPreviewTextureIds),
                  "kBrushPreviewFilenames and BrushPreviewTextureIds must "
                  "stay the same length");
    for (size_t i = 0; i < std::size(kBrushPreviewFilenames); ++i) {
        Asset.BrushPreviewTextureIds[i] =
            UploadPngAsTexture(assetsRoot / "brushes" / kBrushPreviewFilenames[i]);
    }

    // Sprint 7 round-4 -- Upgrade prompt hover-card preview images.
    // Filenames match the UpgradePreview enum in MvStyle.h. Slots whose
    // PNG is missing (e.g. brush_*.png before the artist renames the
    // placeholder copies) load as 0 and silently fall back to the glyph
    // header in the hover card.
    static constexpr const char* kUpgradePreviewFilenames[] = {
        "custom_head.png",
        "texture_albedo.png",
        "texture_full_pbr.png",
        "vertex_mask.png",
        "morph_params.png",
        "brush_grab.png",
        "brush_pushpull.png",
        "brush_inflate.png",
    };
    static_assert(std::size(kUpgradePreviewFilenames) ==
                      std::size(Asset.UpgradePreviewTextureIds),
                  "kUpgradePreviewFilenames and UpgradePreviewTextureIds must "
                  "stay the same length; pair with the UpgradePreview enum");
    for (size_t i = 0; i < std::size(kUpgradePreviewFilenames); ++i) {
        Asset.UpgradePreviewTextureIds[i] =
            UploadPngAsTexture(assetsRoot / "upgrade" / kUpgradePreviewFilenames[i]);
    }
}

void Shutdown() {
    if (Asset.LogoTextureId) {
        GLuint t = Asset.LogoTextureId;
        glDeleteTextures(1, &t);
    }
    for (size_t i = 0; i < std::size(Asset.BrushPreviewTextureIds); ++i) {
        if (Asset.BrushPreviewTextureIds[i]) {
            GLuint t = Asset.BrushPreviewTextureIds[i];
            glDeleteTextures(1, &t);
        }
    }
    for (size_t i = 0; i < std::size(Asset.UpgradePreviewTextureIds); ++i) {
        if (Asset.UpgradePreviewTextureIds[i]) {
            GLuint t = Asset.UpgradePreviewTextureIds[i];
            glDeleteTextures(1, &t);
        }
    }
    Asset = {};
    // Font* lifetimes are owned by the ImFontAtlas; ImGui::DestroyContext
    // (called by AppWindow::Shutdown immediately after this) frees them.
    Font = {};
}

} // namespace MetaVisage::MvStyle
