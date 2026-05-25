---
name: Sprint 4 Story 4.4 Pre-Implementation Audit
description: Paths resolver, Modal harness, MvIcons picker glyphs, AppWindow lifecycle + dispatch sites, user-preset state ownership, JSON I/O pattern, BeginPopupContextItem status
type: project
---

## Key findings

**Paths::AppDataDir** — already exists, `include/utils/Paths.h:9`, `src/utils/Paths.cpp:48–52`. Returns `%APPDATA%\MetaVisage\` with mkdir-on-first-access via `EnsureDir`. No new helper needed; PresetStore calls `AppDataDir() + "/presets"`, mirroring the LogDir pattern at Paths.cpp:54–58.

**Modal harness pattern** — state struct (pendingOpen bool + data fields) owned by AppWindow; free `RenderXxx(State&)` called each frame; `HandlePendingOpen(id, pendingOpen)` at Modals.cpp:19–24 bridges the two. Full signatures in `include/ui/Modals.h:30–125`. No existing text-input modal. Export modal (Modals.cpp:630) is the only `ImGui::InputText` reference — use it as template for Save/Rename modals (char staging buffer, validated before Confirm enables).

**MvIcons picker — 12-glyph set for 4×3 grid**:
- MV_ICON_TARGET "\xEE\x86\x80", MV_ICON_SPARKLES "\xEE\x90\x92", MV_ICON_WAND "\xEE\x8D\x97"
- MV_ICON_CIRCLE "\xEE\x81\xB6", MV_ICON_SQUARE "\xEE\x85\xA7", MV_ICON_BRUSH "\xEE\x8B\xA7"
- MV_ICON_SLIDERS "\xEE\x85\xA2", MV_ICON_CROSSHAIR "\xEE\x82\xAC", MV_ICON_MOVE_3D "\xEE\x8B\xA5"
- MV_ICON_CAMERA "\xEE\x81\xA4", MV_ICON_IMAGE "\xEE\x83\xB6", MV_ICON_SETTINGS "\xEE\x89\x85"
- MV_ICON_STAR not yet in MvIcons.h — add from info.json if 13th+ slot needed.

**AppWindow new-project entry points** (both need QuickWrap apply after make_unique):
- AppWindow::Initialize: AppWindow.cpp:347
- AppWindow::OnFileNewProject: AppWindow.cpp:1856–1863

**Action dispatch block**: AppWindow::Run() ~AppWindow.cpp:755–860. Pattern: `if (actions.xxxRequested) DoSomething()`. New dispatchers for savePreset / rename / delete / replace go in the same block. Confirm modal opened directly: `confirmModal_.Open(title, msg, lambda)` — no PendingAction detour.

**AppWindow modal state ownership**: New `SavePresetModalState savePresetModal_` and `RenamePresetModalState renamePresetModal_` fields on AppWindow; rendered in `RenderModals()`. Follows ConfirmModalState/ErrorModalState convention at AppWindow.h:140–144.

**User preset list ownership**: `std::vector<WrapPreset> userPresets_` directly on AppWindow (consistent with how other lists are held). Pass to sidebar as `const std::vector<WrapPreset>&` parameter on `StageSidebar::Render(...)` — same pattern as `selectedPointIndex` param added at StageSidebar.h:77. Actions struct gains `renamePresetRequested`, `deletePresetRequested`, `replacePresetRequested` bool + `std::string targetPresetId`.

**JSON I/O pattern** (ProjectSerializer.cpp:97–130):
- Write: `std::ofstream out(path)` → `out << root.dump(2)` 
- Read: `std::ifstream in` → `buf << in.rdbuf()` → `Json::parse(buf.str())` in try/catch → `GetOr<T>(root, key, fallback)` for safe reads
- Use `GetOr<int>(obj, "schema_version", 0)` to version-gate on load.

**BeginPopupContextItem**: not used anywhere in src/ui/ — write from scratch. Bind to last item via `BeginPopupContextItem("##ctx")` inside `PushID(i)` / `PopID()` block, user-preset loop only (skip for built-ins).
