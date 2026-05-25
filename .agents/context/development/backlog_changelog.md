# Backlog Changelog

**Project**: MetaVisage
**Purpose**: Append-only archive of completed product backlog items

---

## April 2026

### 2026-04-30 — Stage 4 brush-effect previews shipped (Path A)

- ✅ **Stage 4 procedural brush-effect previews**
  - Sprint 5 Story 5.3 originally shipped brush hotkeys (1–4) and `[N]` indicators in the card sub-labels but deferred the brush-effect previews from the master plan. The deferral was filed as a Medium-priority backlog item.
  - **Effort (Estimated)**: ~1 day (Path B procedural) or ~half day plus asset turnaround (Path A).
  - **Completed**: 2026-04-30 (Sprint 5, Story 5.3 follow-up commit)
  - **Notes**: Path A (PNG assets) landed instead of Path B (procedural ImDrawList). Four brush previews authored externally and staged at `assets/brushes/{smooth,grab,pushpull,inflate}_preview.png`. New optional `ImTextureID thumbnail` parameter on `DrawPresetCard` defaults to 0 — Stage 3 preset cards continue to render Lucide glyphs unchanged; Stage 4 brush cards render the PNGs at 28×28 px with graceful fallback to the Lucide glyph if a load fails. `MvStyle::AssetHandles` extended with a 4-slot `BrushPreviewTextureIds` array; `LoadAssets` / `Shutdown` extended to manage their lifecycle. CMake's existing recursive `file(COPY assets DESTINATION ...)` step picks up `assets/brushes/` automatically on next configure.

---
