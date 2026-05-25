---
name: Sprint 6 Story 6.2 Pre-Implementation Audit
description: Deep-link auth interception wiring — argv shape, SingleInstance pipe details, OpenUrl constraints, TickOnce drain slot, registry/BCrypt gaps
type: project
---

## main.cpp shape (37 lines)

- Line 9: `main(int /*argc*/, char** /*argv*/)` — BOTH argc and argv are discarded today, no argv handling.
- Line 10: Logger init, Line 13: Config::Load(), Lines 15-23: SingleInstance TryAcquire, Line 28: AppWindow::Initialize, Line 34: appWindow.Run().
- Deep-link argv parse must go after Logger+Config (line 13) but BEFORE TryAcquire (line 18): second instance must detect and forward the URI then return 0 before acquiring the mutex.
- Primary-instance cold-launch QueueDeepLink call goes after TryAcquire succeeds (line 23), before or after AppWindow::Initialize — LicenseManager::Initialize() is called inside AppWindow::Initialize at AppWindow.cpp:362, so queueing after Initialize is fine.

## SingleInstance pipe

- Pipe buffer: `kPipeBufferSize = 4096` (SingleInstance.cpp:15)
- Current payload sent by second instance: literal string `"raise"` (main.cpp:18)
- Current onRaise callback: logs only, no focus or URI forwarding (main.cpp:19)
- Listener thread lives for entire app lifetime (singleInstance is a main() local)
- A full `metavisage://auth?code=<64hex>&state=<64hex>` URI is ~150 bytes — no truncation risk

## OpenUrl::Open — scheme whitelist

- Signature: `bool MetaVisage::OpenUrl(const std::string& url)` (include/utils/OpenUrl.h:11)
- Returns false for: empty URL, non-allowed scheme, ShellExecuteW error ≤ 32
- Allowed schemes whitelist (OpenUrl.cpp:29-33): https://, http://, mailto: ONLY
- `https://<api>/desktop-auth/init?state=<hex>` will pass. metavisage:// would be rejected — not relevant since we never call OpenUrl for the deep-link itself.

## TickOnce deep-link drain slot

- TickOnce starts at AppWindow.cpp:673
- Login gate block: lines 693-737 (`if (gateLogin) { ... return; }`)
- LicenseManager instance captured at line 688, GetState() call at line 689
- DRAIN SLOT: insert `licenseMgr.DrainPendingDeepLink()` at line 688 BEFORE the `authState` capture so a transition triggered by the drain is visible in the same frame's GetState() call

## Registry — no existing helper

- No RegCreateKeyExW / RegSetValueExW anywhere in the codebase
- GetModuleFileNameW used in src/utils/Paths.cpp:68 inside Paths::ExecutableDir()
- Convention: wchar_t buf[MAX_PATH], capture DWORD len, convert via local WideToUtf8(buf, len)
- Do NOT call GetModuleFileNameW in LicenseManager.cpp directly — use Paths::ExecutableDir() or add Paths::ExecutablePath() returning the full exe path including filename

## CMake Win32 lib gaps

- No bcrypt, advapi32, or crypt32 in CMakeLists.txt target_link_libraries
- SecureStorage.cpp uses `#pragma comment(lib, "Crypt32.lib")` at line 10 — that is the precedent
- BCryptGenRandom needs bcrypt.lib — NOT transitively guaranteed, must add explicitly
- RegCreateKeyExW needs advapi32.lib — virtually guaranteed transitively but not explicit
- Recommended: add `#pragma comment(lib, "bcrypt.lib")` and `#pragma comment(lib, "advapi32.lib")` in LicenseManager.cpp matching SecureStorage.cpp's pattern, OR add a `if(WIN32)` block in CMakeLists.txt

## Cryptographic randomness — no prior pattern

- PresetStore::GenerateUserPresetId() (src/io/PresetStore.cpp:308-315) is the only existing random generation — uses chrono XOR std::random_device, NOT cryptographically strong
- No BCryptGenRandom or CryptGenRandom call exists anywhere
- Story 6.2 introduces the first CSPRNG call: BCryptGenRandom(nullptr, buf, 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG), hex-encode to 64-char state token

**Why:** These are blocking wiring facts for Story 6.2 that are not derivable from headers alone.
**How to apply:** Use as a checklist when implementing BeginAuthFlow + HandleDeepLink + EnsureSchemeRegistered.
