---
phase: 43-embed-contract-child-window-bootstrap
verified: 2026-05-14T15:52:28Z
status: passed
score: 6/6 must-haves verified
---

# Phase 43: Embed Contract & Child-Window Bootstrap Verification Report

**Phase Goal:** Developer can launch mdCAD as a strict embedded child window from a Windows host.
**Verified:** 2026-05-14T15:52:28Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Embedded launches accept `--embedded --parent-hwnd` in decimal or `0x` form, while unrelated flags remain ignored. | ✓ VERIFIED | `src/app_launch_config.h:27-100`, `src/tests/embed_launch_config_test.c:45-104`, `src/CMakeLists.txt:326-331`, `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test` passed. |
| 2 | Missing, invalid, or incompatible embedded arguments fail fast with a clear startup error and no silent fallback path. | ✓ VERIFIED | `src/app.c:2519-2527` exits before returning `sapp_desc`; `src/platform/win32_embed.h:38-45` exits with embed-specific stderr; patched `sokol_app.h:9747-9795` rejects invalid parent/client/DC cases; host captures `Embedded startup failed:` in `samples/avalonia-host/MainWindow.axaml.cs:344-377`. |
| 3 | Embedded mdCAD is created as a true Win32 child window under the supplied parent HWND from birth, not via late reparenting. | ✓ VERIFIED | Patch uses `WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN` and `hWndParent = parent_hwnd` in `vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch:17-85`; configured vendor source shows same in `build-vulkan/_deps/libsokol-src/sokol_app.h:9738-9795`; no `SetParent` path exists. |
| 4 | A minimal Avalonia host exists, builds, and launches mdCAD into a `NativeControlHost` child region using the embed contract. | ✓ VERIFIED | `samples/avalonia-host/AvaloniaHost.csproj:1-15`, `samples/avalonia-host/MainWindow.axaml:10-27`, `samples/avalonia-host/MainWindow.axaml.cs:282-341`, and `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` succeeded. |
| 5 | The host surfaces the requested bootstrap states and detects attach/failure from real process/window data. | ✓ VERIFIED | `samples/avalonia-host/MainWindow.axaml.cs:137-140, 339-419, 491-541` updates UI to `launching`, `waiting for child attach`, `attached`, `timeout/failure`, traces child HWND via `EnumChildWindows` + `GetWindowThreadProcessId`, and syncs child bounds. |
| 6 | Phase evidence includes completed valid/invalid/destroyed parent-HWND checks with no standalone fallback recorded. | ✓ VERIFIED | `.planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md:1-52` has no `TBD`, ends with `Checklist verdict: PASS`, and records explicit invalid/destroyed parent failure evidence. |

**Score:** 6/6 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/app_launch_config.h` | Strict embed launch parser | ✓ VERIFIED | Parses `--embedded` / `--parent-hwnd`, accepts decimal+hex, rejects duplicates/missing values. |
| `src/tests/embed_launch_config_test.c` | Parser regression coverage | ✓ VERIFIED | Covers no-embed, hex, decimal, missing parent, orphan parent, invalid HWND, duplicate embed. |
| `src/CMakeLists.txt` | Registers parser contract test | ✓ VERIFIED | `embed_launch_config_test` target and CTest entry exist. |
| `src/platform/win32_embed.h` | Shared Win32 embed startup state + fail-fast helper | ✓ VERIFIED | Exposes config handoff, parent HWND lookup, and `mdcad_win32_embed_fail_startup()`. |
| `vendors/libsokol/CMakeLists.txt` | Pinned Sokol revision + checked-in patch application seam | ✓ VERIFIED | Pins commit `2356d22...`, applies patch via `git apply --recount`, exposes `src/` includes. |
| `vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch` | Child-window bootstrap patch | ✓ VERIFIED | Uses `WS_CHILD` birth path, validates parent HWND, no `SetParent`. |
| `build-vulkan/_deps/libsokol-src/sokol_app.h` | Configured vendor source reflects applied patch | ✓ VERIFIED | Patched include and embedded `CreateWindowExW` branch are present in configured build tree. |
| `src/app.c` | Pre-window parse + embed state handoff + embedded defaults | ✓ VERIFIED | Strict parser wiring and debug-window defaults are present; note `state.ui_visible` remains `true`, so an earlier plan-level chrome-trim expectation is not implemented. |
| `samples/avalonia-host/AvaloniaHost.csproj` | Buildable Windows Avalonia host | ✓ VERIFIED | Restores/builds and references `Avalonia.Desktop`. |
| `samples/avalonia-host/MainWindow.axaml` | Minimal UI with `NativeControlHost` and status text | ✓ VERIFIED | Declares `EmbedSurface`, `StatusTextBlock`, `ModeTextBlock`, `FailureTextBlock`. |
| `samples/avalonia-host/MainWindow.axaml.cs` | Auto-launch, attach detection, failure capture, cleanup | ✓ VERIFIED | Launches mdCAD with embed args, captures stderr, polls attach, syncs child size, kills child on close. |
| `.planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md` | Recorded attach/failure proof | ✓ VERIFIED | All scenarios filled and overall verdict is `PASS`. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `src/app.c` | `src/app_launch_config.h` | `mdcad_launch_config_parse(...)` before returning `sapp_desc` | ✓ WIRED | `src/app.c:2519-2527` calls parser and exits on error. |
| `src/app.c` | `src/platform/win32_embed.h` | `mdcad_win32_embed_set_config(&launch_config)` | ✓ WIRED | Parsed launch config is handed off before window creation. |
| `vendors/libsokol/CMakeLists.txt` | `vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch` | checked-in patch application seam | ✓ WIRED | CMake applies patch to pinned vendor source during configure. |
| configured `sokol_app.h` | `src/platform/win32_embed.h` | embedded `CreateWindowExW` branch | ✓ WIRED | Patched Sokol source includes `platform/win32_embed.h` and calls embed helpers. |
| `samples/avalonia-host/MainWindow.axaml.cs` | `src/app_launch_config.h` contract | launch args `--embedded --parent-hwnd 0x{hwnd}` | ✓ WIRED | `Arguments = $"--embedded --parent-hwnd 0x{...}"`. |
| `samples/avalonia-host/MainWindow.axaml.cs` | `samples/avalonia-host/MainWindow.axaml` | named controls for status/failure/embed surface | ✓ WIRED | Code resolves and updates `StatusTextBlock`, `ModeTextBlock`, `FailureTextBlock`, `EmbedSurface`. |
| `43-MANUAL-CHECKLIST.md` | `samples/avalonia-host/MainWindow.axaml.cs` | harness modes + captured embed failure text | ✓ WIRED | Checklist scenarios reference `--embed-test-mode` variants and `Embedded startup failed:` evidence reflected by host code. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `samples/avalonia-host/MainWindow.axaml.cs` | `_statusTextBlock.Text` | `LaunchEmbeddedViewer()`, `OnAttachTimerTick()`, `SetFailureStatus()` | Yes — status changes depend on process start, timeout, and child-window detection. | ✓ FLOWING |
| `samples/avalonia-host/MainWindow.axaml.cs` | `_capturedFailureLine` / `_failureTextBlock.Text` | redirected stdout/stderr in `OnChildOutputDataReceived()` | Yes — failure text comes from mdCAD stderr lines containing `Embedded startup failed:`. | ✓ FLOWING |
| `samples/avalonia-host/MainWindow.axaml.cs` | `_attachedChildHwnd` | `FindChildWindowForProcess()` via `EnumChildWindows` + `GetWindowThreadProcessId` | Yes — attach state depends on real HWND ownership under the host parent. | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Parser contract regression | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test` | `100% tests passed` | ✓ PASS |
| Native embed build path | `cmake --build build-vulkan --config Release --target mdCAD` | `mdCAD.exe` built successfully | ✓ PASS |
| Avalonia host build | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | `Build succeeded` | ✓ PASS |
| Manual checklist completion gate | `python -c "... assert 'TBD' not in t ..."` | `checklist-ok` | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| `EMBD-01` | `43-01` | Developer can launch mdCAD in Windows embedded mode by passing a parent HWND on the command line. | ✓ SATISFIED | Parser accepts decimal/hex HWND values in `src/app_launch_config.h`; host launches mdCAD with `--embedded --parent-hwnd` in `samples/avalonia-host/MainWindow.axaml.cs:296-305`. |
| `EMBD-02` | `43-02` | Embedded mdCAD creates and remains inside a true child window under the supplied parent HWND instead of silently falling back to a standalone top-level window. | ✓ SATISFIED | Patched Sokol path uses `WS_CHILD` birth creation with `hWndParent = parent_hwnd`; host detects attach via child enumeration; checklist records no standalone fallback. |
| `EMBD-03` | `43-01`, `43-02`, `43-03` | Developer gets a clear startup failure when embedded-mode arguments are missing, invalid, or incompatible. | ✓ SATISFIED | `src/app.c` and `src/platform/win32_embed.h` fail with explicit `Embedded startup failed:` errors; host surfaces captured stderr; checklist covers invalid/destroyed parent HWND modes. |
| `HOST-01` | `43-01`, `43-03` | Developer can build and run a minimal Avalonia sample that embeds mdCAD inside a `NativeControlHost`. | ✓ SATISFIED | Avalonia sample builds successfully, uses `NativeControlHost`, launches mdCAD with embed args, and recorded checklist evidence shows valid attach. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `src/app.c` | 1531 | `state.ui_visible = true` instead of the earlier `43-02` plan's `state.ui_visible = false` expectation | ℹ️ Info | This diverges from one plan-level viewer-trim detail, but it does not block Phase 43's required embed contract or host-child bootstrap goal. |

### Human Verification Required

None. The phase's manual-only windowing checks were already captured in `43-MANUAL-CHECKLIST.md`, and automated build/test checks align with that evidence.

### Gaps Summary

No goal-blocking gaps found. The codebase contains a strict embed launch contract, a patched true child-window bootstrap path, and a buildable Avalonia `NativeControlHost` sample wired to launch mdCAD with attach/failure reporting. The only notable deviation is that embedded startup now keeps the requested mdCAD panels visible (`state.ui_visible = true`) instead of the earlier plan's chrome-trim expectation, which does not block `EMBD-01`, `EMBD-02`, `EMBD-03`, or `HOST-01`.

---

_Verified: 2026-05-14T15:52:28Z_
_Verifier: the agent (gsd-verifier)_
