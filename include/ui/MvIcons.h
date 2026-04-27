#ifndef METAVISAGE_UI_MVICONS_H
#define METAVISAGE_UI_MVICONS_H

// Sprint 3.8.5+ — Lucide icon font glyph constants.
//
// Codepoints sourced from the canonical Lucide GitHub release `info.json`
// bundled in assets/fonts/. Glyphs sit in the Unicode Private Use Area
// (U+E000–U+F8FF). Each constant is a UTF-8 string literal so it can be
// passed directly to ImGui::Text / ImDrawList::AddText after pushing
// MvStyle::Font.Icon (or IconSm).
//
// Usage pattern:
//
//   { MvStyle::ScopedFont f(MvStyle::Font.Icon);
//     ImGui::Text(MV_ICON_FOLDER); }
//
// Or via ImDrawList:
//
//   dl->AddText(MvStyle::Font.Icon, 16.0f, pos, color, MV_ICON_EYE);
//
// Add new constants here as stories surface new icon needs — extract the
// codepoint from assets/fonts/info.json (lookup by the icon's lucide.dev
// name), then add the matching `MV_ICON_*` constant below.

// =============================================================================
// Window chrome (Story 3.8.2)
// =============================================================================
#define MV_ICON_X            "\xEE\x86\xB2"  // U+E1B2  — close
#define MV_ICON_MINIMIZE     "\xEE\x84\x9B"  // U+E11B  — minimize-2
#define MV_ICON_MAXIMIZE     "\xEE\x84\x93"  // U+E113  — maximize-2

// =============================================================================
// Navigation / chevrons (Stories 3.8.3, 3.8.7+)
// =============================================================================
#define MV_ICON_CHEVRON_UP    "\xEE\x81\xB0"  // U+E070  — chevron-up
#define MV_ICON_CHEVRON_DOWN  "\xEE\x81\xAD"  // U+E06D  — chevron-down
#define MV_ICON_CHEVRON_LEFT  "\xEE\x81\xAE"  // U+E06E  — chevron-left
#define MV_ICON_CHEVRON_RIGHT "\xEE\x81\xAF"  // U+E06F  — chevron-right
#define MV_ICON_CHECK         "\xEE\x81\xAC"  // U+E06C  — check
#define MV_ICON_CHECK_CIRCLE  "\xEE\x88\xA6"  // U+E226  — circle-check

// =============================================================================
// Stage 1 — Alignment (Story 3.8.4 rail + 3.8.7 inspector)
// =============================================================================
// 3D-axis variants are the canonical transform-tool glyphs — they read as
// gizmo handles, not 2D arrows. Used by the tool rail in Stage 1.
#define MV_ICON_MOVE_3D    "\xEE\x8B\xA5"  // U+E2E5  — move-3d
#define MV_ICON_ROTATE_3D  "\xEE\x8B\xAA"  // U+E2EA  — rotate-3d
#define MV_ICON_SCALE_3D   "\xEE\x8B\xAB"  // U+E2EB  — scale-3d
// Legacy 2D variants kept for inline contexts where the 3D form would feel
// too heavy (e.g. the Edit menu's "Move" entry, if we ever add one).
#define MV_ICON_MOVE       "\xEE\x84\xA1"  // U+E121  — move (4-arrow)
#define MV_ICON_ROTATE     "\xEE\x85\x89"  // U+E149  — rotate-cw
#define MV_ICON_SCALE      MV_ICON_SCALE_3D
#define MV_ICON_RESET      "\xEE\x85\x88"  // U+E148  — rotate-ccw (reset)
#define MV_ICON_REFRESH    "\xEE\x85\x85"  // U+E145  — refresh-cw
#define MV_ICON_WAND       "\xEE\x8D\x97"  // U+E357  — wand-2 (auto-align)
#define MV_ICON_SPARKLES   "\xEE\x90\x92"  // U+E412  — sparkles

// =============================================================================
// Viewport toolbar / shading + visibility (Story 3.8.5)
// =============================================================================
#define MV_ICON_EYE         "\xEE\x82\xBA"  // U+E0BA  — eye
#define MV_ICON_EYE_OFF     "\xEE\x82\xBB"  // U+E0BB  — eye-off
#define MV_ICON_CIRCLE      "\xEE\x81\xB6"  // U+E076  — circle (solid shading)
#define MV_ICON_BOX         "\xEE\x81\xA1"  // U+E061  — box (wireframe shading)
#define MV_ICON_IMAGE       "\xEE\x83\xB6"  // U+E0F6  — image (textured shading)
#define MV_ICON_CAMERA      "\xEE\x81\xA4"  // U+E064  — camera (perspective)
#define MV_ICON_SQUARE      "\xEE\x85\xA7"  // U+E167  — square
#define MV_ICON_EXPAND      "\xEE\x88\x9A"  // U+E21A  — expand
#define MV_ICON_CROSSHAIR   "\xEE\x82\xAC"  // U+E0AC  — crosshair (pivot snap)
#define MV_ICON_TARGET      "\xEE\x86\x80"  // U+E180  — target
#define MV_ICON_SLIDERS     "\xEE\x85\xA2"  // U+E162  — sliders
#define MV_ICON_FLIP_HORIZ  "\xEE\x8D\x9D"  // U+E35D  — flip-horizontal (symmetry X)
#define MV_ICON_CIRCLE_DASHED "\xEE\x92\xB0"  // U+E4B0  — circle-dashed (Stage 3 source view)

// =============================================================================
// File / project (Stories 3.8.7, 3.8.11)
// =============================================================================
#define MV_ICON_FOLDER       "\xEE\x83\x97"  // U+E0D7  — folder
#define MV_ICON_FOLDER_OPEN  "\xEE\x89\x87"  // U+E247  — folder-open
#define MV_ICON_FILE         "\xEE\x83\x80"  // U+E0C0  — file
#define MV_ICON_FILE_TEXT    "\xEE\x83\x8C"  // U+E0CC  — file-text
#define MV_ICON_PLUS         "\xEE\x84\xBD"  // U+E13D  — plus
#define MV_ICON_MINUS        "\xEE\x84\x9C"  // U+E11C  — minus
#define MV_ICON_TRASH        "\xEE\x86\x8E"  // U+E18E  — trash-2
#define MV_ICON_UPLOAD       "\xEE\x86\x9E"  // U+E19E  — upload
#define MV_ICON_DOWNLOAD     "\xEE\x82\xB2"  // U+E0B2  — download
#define MV_ICON_IMPORT       "\xEE\x88\xAF"  // U+E22F  — import
#define MV_ICON_COPY         "\xEE\x82\x9E"  // U+E09E  — copy

// =============================================================================
// Stage 4 — Touch Up brushes (Story 3.8.4 rail + 3.8.10 inspector)
// =============================================================================
#define MV_ICON_BRUSH         "\xEE\x8B\xA7"  // U+E2E7  — paintbrush (smooth)
#define MV_ICON_BRUSH_VERT    "\xEE\x8B\xA8"  // U+E2E8  — paintbrush-vertical
#define MV_ICON_HAND          "\xEE\x87\x97"  // U+E1D7  — hand (grab)
#define MV_ICON_MOVE_VERTICAL "\xEE\x87\x87"  // U+E1C7  — move-vertical (push/pull)

// =============================================================================
// Status / semantic (Stories 3.8.7+)
// =============================================================================
#define MV_ICON_INFO          "\xEE\x83\xB9"  // U+E0F9  — info
#define MV_ICON_ALERT         "\xEE\x86\x93"  // U+E193  — triangle-alert
#define MV_ICON_HELP          "\xEE\x82\x82"  // U+E082  — circle-help
#define MV_ICON_LOCK          "\xEE\x84\x8B"  // U+E10B  — lock
#define MV_ICON_EXTERNAL      "\xEE\x82\xB9"  // U+E0B9  — external-link
#define MV_ICON_SETTINGS      "\xEE\x89\x85"  // U+E245  — settings-2

#endif // METAVISAGE_UI_MVICONS_H
