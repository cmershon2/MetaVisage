---
name: Sprint 6 Story 6.3 Pre-Implementation Audit
description: Code exchange + JWT verify + session persistence — HttpClient threading, morph-worker pattern, Config thread-safety, Logger thread-safety, jwt-cpp integration, curl SSL backend, modal call-site rules
type: project
---

## HttpClient threading + timeouts

- Thread-safe: each PostJson call creates its own CURL* easy handle (HttpClient.cpp:53).
- Global init: `std::once_flag g_curlInitOnce` + `std::call_once` in EnsureGlobalInit() (HttpClient.cpp:15-22). Called from constructor (line 98) and from Execute() (line 50). No external init needed.
- Timeouts: CURLOPT_TIMEOUT=30s (line 64), CURLOPT_CONNECTTIMEOUT=10s (line 65). Will not block forever.
- CURLOPT_FOLLOWLOCATION=1 is set (line 66) — redirects are followed automatically.

## Worker-thread pattern (morph worker — the canonical model)

- No separate class. Everything lives in AppWindow.
- Members (AppWindow.h:180-190): `std::thread morphThread_`, `std::atomic<bool> morphRunning_`, `std::atomic<bool> morphFinished_`, `std::mutex morphResultMutex_`, raw result fields (success bool, error string, shared_ptr<Mesh>, vectors).
- StartMorph (AppWindow.cpp:2384): joins stale thread, constructs work object, sets atomics false, spawns `morphThread_ = std::thread([this]() { ... })`. Lambda writes results under morphResultMutex_, then sets morphFinished_.store(true).
- PollMorphCompletion (AppWindow.cpp:2486): checks morphFinished_.load(); if false, returns immediately. If true, joins thread, moves results out under mutex, then on main thread: calls errorModal_.Open() on failure or commits to Project on success.
- Polling call sites: TickOnce line 733 (inside login-gate early-return path) and line 781 (main render path). Both call PollMorphCompletion every frame.
- Pattern to mirror: one std::thread, atomic<bool> running+finished, one mutex protecting result struct, poll in TickOnce, no worker-side UI or Config calls.

## Config::SetTokens / SetLicenseJwt — NOT thread-safe

- Both call Save() synchronously (Config.cpp:191, Config.cpp:203).
- Config has NO internal mutex — methods are completely unguarded.
- Save() calls SecureStorage::EncryptString + opens/writes/closes config.json inline.
- MUST marshal SetTokens + SetLicenseJwt to the main thread. Worker thread writes exchange result into a mutex-protected struct; TickOnce drain calls Config on main thread after joining.

## MV_LOG_* macros — thread-safe

- Logger::Log() takes std::lock_guard<std::mutex>(mutex_) as first statement (Logger.cpp:94). Covers both file write and console write.
- Logger::Instance() is a Meyer's singleton (Logger.cpp:48) — safe under C++11+ static init rules.
- MV_LOG_* macros can be freely called from the exchange worker thread.

## jwt-cpp integration in vcpkg

- No jwt-cpp or picojson in vcpkg.json today (vcpkg.json has 7 deps).
- Header-only vcpkg precedent: Eigen3::Eigen (CMakeLists.txt:315) and nlohmann_json::nlohmann_json (line 312). Both use find_package CONFIG + target_link_libraries with the interface target — no source file or add_library.
- Pattern for jwt-cpp: add to vcpkg.json, find_package(jwt-cpp CONFIG REQUIRED) near lines 36-72, link jwt-cpp::jwt-cpp in target_link_libraries for MetaVisageCore (lines 306-316).

## curl SSL backend = Schannel (NOT OpenSSL)

- curl version 8.19.0 built with SSPI/Schannel, not OpenSSL.
- CURLConfig.cmake:60-73: the if("") block around find_dependency(OpenSSL) evaluates false — OpenSSL was NOT a curl build dep.
- No libssl.dll or libcrypto.dll in build/vcpkg_installed/x64-windows/bin/.
- CURL_SUPPORTED_FEATURES includes SSPI and SSL but not OpenSSL.
- Consequence: jwt-cpp with OpenSSL backend requires explicitly adding "openssl" to vcpkg.json + find_package(OpenSSL REQUIRED) + link OpenSSL::SSL + OpenSSL::Crypto in CMakeLists.txt. Adds libssl.dll + libcrypto.dll to deploy bundle.
- Alternative: jwt-cpp with Botan or mbedTLS backend avoids OpenSSL, but OpenSSL is the most-tested path.

## ErrorModalState — main-thread only

- errorModal_.Open() is never called from worker lambdas — only from main-thread functions (StartMorph pre-spawn validation, PollMorphCompletion post-join).
- AppWindow.h:157: `ErrorModalState errorModal_` is a plain value member (not atomic, not mutex-guarded).
- Exchange worker must store error into mutex-protected result field; TickOnce drain calls errorModal_.Open() on main thread.

## LicenseConfig public key shape

- cmake/LicenseConfig.h.in:23: key is an RS256 SPKI PEM embedded as a raw string literal in `inline constexpr const char* kLicenseJwtPublicKey`.
- Empty in development builds; CMake refuses production builds with an empty key (CMakeLists.txt:137-143).
- jwt-cpp RS256 verify will need to handle the empty-key case in development (skip verification or return a dev-only grant).

**Why:** Blocking implementation facts for Story 6.3 not derivable from headers alone.
**How to apply:** Use as a checklist when implementing ExchangeCode(), VerifyLicenseJwt(), PersistSession(), and the TickOnce drain slot.
