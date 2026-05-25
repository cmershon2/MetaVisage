---
name: Sprint 7 Pre-Implementation Audit
description: Feature gating and conversion tracking audit — LicenseManager claim state, sidebar surfaces, status bar, modal, AcceptMorph hook, smoke_ui pattern
type: project
---

Key findings:

**LicenseManager claims (LicenseManager.h + .cpp)**
- Parses: tier_, licenseKey_, conversionsUsed_, conversionsLimit_, conversionsResetAt_ — done in LoadClaimsFromJwt at lines 557-589
- feature_flags object is NOT parsed at all — no field in the struct, no jwt-cpp read for it
- No typed getters like CanUploadCustomHead() or GetTextureTransferMode() exist yet
- GetTier() / GetConversionsUsed() / GetConversionsLimit() are the only gating hooks today (LicenseManager.h:100-105)

**Conversion endpoint**
- No ReportConversion() or POST /api/license/conversion anywhere in codebase
- Pattern to mirror: ExchangeResult + exchangeThread_ + exchangeFinished_ + DrainPendingExchange
- Retry budget constants live at LicenseManager.h:170-171 (kExchangeMaxAttempts=4, kExchangeBudgetSeconds=30)
- RefreshOutcome enum + pendingRefreshResult_ is the offline-queue analog in LicenseManager.h:253-274

**AcceptMorph — conversion trigger point**
- AppWindow.cpp:895 dispatches acceptMorphRequested → AcceptMorph()
- AcceptMorph() body at AppWindow.cpp:2643-2648: sets morphData.isAccepted = true, nothing else
- ReportConversion() call slots in right here after the isAccepted flip

**Stage 1 mesh-load buttons**
- StageSidebar.cpp:159 RenderAlignment(); mesh slot buttons at ~line 197 (morph) and ~220 (target)
- importMorphMeshRequested / importTargetMeshRequested set in Actions struct (StageSidebar.h:24-25)
- AppWindow dispatches to NFD picker via the Actions struct each frame

**Stage 1 texture upload**
- StageSidebar.cpp:222-253: "TARGET TEXTURES" section; combo of kSlotNames[] (Albedo/Normal/etc.) + "+ Texture" button
- Sets out.importTextureSlot (StageSidebar.h:49); AppWindow opens NFD and calls project texture load

**Stage 3 parameters — gating surface**
- Preset cards: DrawPresetCard() loop ~line 995-1005 (built-ins), then user presets
- activePresetId_ auto-derived by param equality at ~line 948; cleared on any paramEdited (line 1223)
- General collapsible: DrawCollapsibleSection("morph.general",...) at line 1140 — sliders inside
- Advanced collapsible: DrawCollapsibleSection("morph.advanced",...) at line 1164 — OPTIMIZATION/RIGIDITY/CONTROL NODE SAMPLING sub-sections
- paramEdited flag → activePresetId_.clear() is the natural "free tier locks sliders" injection point

**Stage 3 vertex masking**
- DrawSectionLabel("VERTEX MASK") at StageSidebar.cpp:1058
- Checkbox "Paint mask" at line 1069; maskPaintingActive_ controls the gate

**Stage 4 brush selector**
- Inspector cards: 2x2 table loop in RenderTouchUp() at StageSidebar.cpp:1338-1354; sculpt.activeBrush set on click
- ToolRail: kStage4Tools[] at ToolRail.cpp:42, Render() loop at ~line 229-248; brush click sets viewport->SetBrushType() AND project->GetSculptState().activeBrush

**Status bar**
- RenderStatusBar() in AppWindow.cpp:1599-1724
- Left cluster (left to right): ● Ready | [Offline pill if IsOfflineMode()] | Stage NN NAME | Source | Target | Tier UPPER
- Right cluster: GPU label only (line 1711)
- drawSection() + drawSep() inline lambdas; new "X conversions remaining" pill inserts after Tier at ~line 1703

**License Status modal**
- RenderLicenseModal() at Modals.cpp:1002; opened via licenseModal_.Open() at AppWindow.cpp:1800
- GetConversionsUsed()/GetConversionsLimit() consumed at Modals.cpp:1037-1040
- LicenseStatusPill() helper at Modals.cpp:957

**smoke_ui**
- tests/smoke/ui_smoke.cpp — 4 scenarios registered at main() lines 239-242
- Pattern: RegisterScenario_X(engine) free function, IM_REGISTER_TEST macro, TestFunc lambda
- g_app static pointer used to reach AppWindow/Viewport state from within tests
- New scenario = new RegisterScenario_X() function + call in main() before ImGuiTestEngine_Start
