---
phase: 44-embedded-resize-focus-viewer-layout
verified: 2026-05-14T20:03:09.9504849Z
status: human_needed
score: 2/5 must-haves verified
human_verification:
  - test: "Scenario 2 — first-click keyboard ownership"
    expected: "Embedded attach does not auto-focus mdCAD; one deliberate click gives mdCAD keyboard ownership; Tab stays in mdCAD; clicking host UI returns focus to the host."
    why_human: "The reducer/shortcut gate is wired in code, but the only recorded UAT result is the pre-fix failure in 44-UAT.md and cross-process focus behavior is not proven by unit tests."
  - test: "Scenario 5 — destroyed-parent self-exit"
    expected: "Destroyed-parent mode reports mdCAD self-exit before any host fallback kill, with no save prompt, no standalone fallback, and no surviving mdCAD.exe."
    why_human: "The host now waits for graceful exit and mdCAD still has the invalid-parent quit guard, but the recorded checklist/UAT evidence is still the pre-fix fallback-cleanup outcome."
  - test: "Scenario 7 — embedded layout persistence and standalone isolation"
    expected: "Embedded launches seed the viewer-first layout, persist rearranged panels via build-vulkan\\bin\\Release\\imgui.embedded.ini, and standalone mdCAD continues using imgui.ini independently."
    why_human: "Code now resolves the embedded store path and waits on host close, but layout persistence/isolation remains a visual multi-launch behavior that has not been re-run after 44-06."
---

# Phase 44: Embedded Resize, Focus & Viewer Layout Verification Report

**Phase Goal:** Users can interact with the embedded viewer correctly inside the host lifecycle.
**Verified:** 2026-05-14T20:03:09.9504849Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | User can resize the host control and mdCAD resizes its hosted render surface without clipped, stale, or incorrect viewport behavior. | ✓ VERIFIED | `samples/avalonia-host/MainWindow.axaml.cs` keeps host-owned resize sync; prior UAT Scenario 1 passed; no gap-closure regression seen in code. |
| 2 | User can click into the embedded viewer and immediately use mdCAD keyboard/mouse interactions without host interference. | ? UNCERTAIN | `src/app.c:336-344` now gates embedded shortcuts via `embed_input_state_allows_shortcuts(&state.embed_input)` and `src/app.c:2085-2109` still guards the shortcut block, but recorded UAT Scenario 2 is still the pre-fix failure. |
| 3 | User can move focus between host UI and mdCAD without stuck capture, stuck drag, or broken input state. | ✓ VERIFIED | Prior UAT Scenario 3 passed; `src/app.c:346-360` still cancels on runtime invalidation and `src/app.c:2744-2763` routes unfocus/deactivate through the embedded reducer/cancel path. |
| 4 | Embedded mdCAD shuts down cleanly when the host/control lifecycle ends or the parent HWND becomes invalid. | ? UNCERTAIN | `samples/avalonia-host/MainWindow.axaml.cs:483-680` now waits for graceful exit before fallback kill and `src/app.c:356-360` still triggers `sapp_quit()` on invalid parent chain, but destroyed-parent evidence has not been re-run after the fix. |
| 5 | Embedded launches use a viewer-first layout that persists separately from standalone `imgui.ini`. | ? UNCERTAIN | `src/embed_layout_state.h:30-48` still splits embedded vs standalone policy and `src/imgui_storage.h:238-359,402-477` resolves/loads/saves `imgui.embedded.ini`, but Scenario 7 evidence remains the pre-fix failure. |

**Score:** 2/5 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `samples/avalonia-host/MainWindow.axaml.cs` | Launch embedded mdCAD, keep child bounds synced, and prefer graceful teardown before fallback kill | ✓ VERIFIED | `WorkingDirectory` is set to the mdCAD exe directory (`300-315`); resize/lifecycle handlers exist; destroyed-parent and host-close paths wait before `Kill(true)` (`483-680`, `810-829`). |
| `src/app.c` | Embedded shortcut gating, focus/cancel handling, invalid-parent quit, and layout save hooks | ✓ VERIFIED | Embedded shortcuts now follow reducer-owned focus (`336-344`), shortcut block remains wired (`2085-2109`), invalid-parent guard calls `sapp_quit()` (`346-360`), and save hooks run on unfocus/shutdown (`1725-1766`, `2699-2760`). |
| `src/imgui_storage.h` | Resolve, load, and save the embedded manual store at the runtime path | ✓ VERIFIED | Relative `imgui.embedded.ini` is resolved to an explicit desktop path via `_fullpath`/cwd (`238-285`) and used by load/frame/shutdown save paths (`309-359`, `402-477`). |
| `src/embed_layout_state.h` | Keep embedded persistence separate from standalone `imgui.ini` | ✓ VERIFIED | Embedded policy still uses manual persistence with `imgui.embedded.ini`; standalone still uses `imgui.ini` (`30-48`). |
| `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` | Point Scenario 7 at the real embedded store path and drive final human checks | ⚠️ WARNING | Commands now target `build-vulkan\\bin\\Release\\imgui.embedded.ini`, but observed states/verdicts still reflect the pre-fix run (`119-133`). |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `samples/avalonia-host/MainWindow.axaml.cs` | `mdCAD.exe` | `ProcessStartInfo` with `--embedded --parent-hwnd ...` and exe-directory `WorkingDirectory` | ✓ WIRED | Launch path is present at `306-315`. |
| `samples/avalonia-host/MainWindow.axaml.cs` | Teardown outcome | graceful wait first, fallback `Kill(true)` only on verified failure | ✓ WIRED | `BeginGracefulTeardownWait`, `ObserveGracefulExitAsync`, `WaitForGracefulExitOnClose`, and `EnsureAttachedProcessTeardown` are connected at `518-657`. |
| `src/app.c` | `state.embed_input` | `mdcad_embedded_shortcuts_allowed()` uses reducer-owned keyboard state | ✓ WIRED | `return embed_input_state_allows_shortcuts(&state.embed_input);` at `343`. |
| `src/app.c` | ImGui shortcut handling | `if (mdcad_embedded_shortcuts_allowed(io))` guards the keyboard shortcut block | ✓ WIRED | Shortcut gate is used at `2085-2086` and still covers `Tab` at `2104-2109`. |
| `src/app.c` | mdCAD quit path | invalid parent chain triggers `sapp_quit()` | ✓ WIRED | `mdcad_win32_embed_parent_chain_valid(...)` feeds `sapp_quit()` at `356-360`. |
| `src/app.c` + `src/imgui_storage.h` | Embedded layout persistence | `imgui_storage_configure/init/shutdown` around embedded layout policy | ✓ WIRED | Policy resolve/configure occurs before `simgui_setup()` (`1730-1736`); load/save hooks run on init/unfocus/shutdown (`1765-1766`, `2699-2700`, `2758-2760`). |
| `44-MANUAL-CHECKLIST.md` | `build-vulkan\\bin\\Release\\imgui.embedded.ini` | explicit reset/inspection commands | ✓ WIRED | Scenario 7 commands reference the runtime file path at `119-121`. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/app.c` | `state.embed_input` | `event(...)` sends focus/mouse/unfocus triggers to `mdcad_handle_embedded_input_trigger(...)`, then `mdcad_embedded_shortcuts_allowed(...)` reads reducer state | Yes | ✓ FLOWING |
| `src/app.c` + `src/imgui_storage.h` | `state.embed_layout.manual_store_filename` | `embed_layout_state_resolve(...)` -> `imgui_storage_configure(...)` -> resolved runtime path -> `imgui_storage_native_load/save(...)` | Yes | ✓ FLOWING |
| `samples/avalonia-host/MainWindow.axaml.cs` | `_mdcadProcess` teardown status | Real process lifecycle from `Process.Start()` -> `WaitForExitAsync/WaitForExit()` -> `FinalizeTeardownStatus(...)` | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Phase 44 native regression targets build and pass | `cmake --build build-vulkan --config Release --target mdCAD embed_input_state_test embed_layout_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_launch_config_test|embed_input_state_test|embed_layout_state_test"` | 3/3 tests passed; `mdCAD.exe`, `embed_input_state_test.exe`, and `embed_layout_state_test.exe` built successfully | ✓ PASS |
| Avalonia host still builds after teardown changes | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | Build succeeded in 3.7s | ✓ PASS |
| Gap-closure docs point Scenario 7 at the real embedded store path | `python -c "from pathlib import Path; ..."` | `checklist-path-and-close-note-ok` | ✓ PASS |
| Runtime wiring for keyboard ownership and graceful teardown exists in code | `python -c "from pathlib import Path; ..."` | `app-shortcuts-and-storage-hooks-ok`; `host-graceful-exit-hooks-ok` | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| `EMBD-04` | `44-01`, `44-02`, `44-03`, `44-06` | Embedded mdCAD shuts down cleanly when the host/control lifecycle ends or the parent HWND becomes invalid. | ? NEEDS HUMAN | mdCAD still has the invalid-parent `sapp_quit()` guard (`src/app.c:356-360`) and the host now prefers graceful exit before fallback kill (`MainWindow.axaml.cs:518-657`), but Scenario 5 has not been re-run after the fix. |
| `INPT-01` | `44-01`, `44-03` | Host resize keeps the embedded render surface correct. | ✓ SATISFIED | Prior UAT Scenario 1 passed and resize sync remains in the host lifecycle code. |
| `INPT-02` | `44-01`, `44-02`, `44-05` | First click grants mdCAD interactive ownership without host interference. | ? NEEDS HUMAN | `mdcad_embedded_shortcuts_allowed(...)` now follows reducer state (`src/app.c:336-344`), but Scenario 2 has only pre-fix failing evidence in `44-UAT.md`. |
| `INPT-03` | `44-01`, `44-02` | Focus/deactivation changes do not leave stuck capture or drag state. | ✓ SATISFIED | Prior UAT Scenario 3 passed and cancel/reset wiring remains in `src/app.c:346-360` and `2744-2760`. |
| `INPT-04` | `44-01`, `44-04`, `44-06` | Embedded viewer uses a viewer-first layout that persists separately from standalone layout. | ? NEEDS HUMAN | Embedded/standalone storage split still exists in `src/embed_layout_state.h` and `src/imgui_storage.h`, but Scenario 7 remains visually unverified after the teardown/path fix. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `.planning/phases/44-embedded-resize-focus-viewer-layout/44-UAT.md` | 25-27, 39-50 | Stale pre-fix manual evidence still marks Scenarios 2, 5, and 7 as issues | ⚠️ Warning | Verification cannot promote the phase to `passed` until those human scenarios are re-run. |
| `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` | 83-85, 123-133 | Commands were updated, but observed states/verdicts still reflect the pre-fix run | ⚠️ Warning | The checklist is usable for re-test execution, but it is not fresh evidence of gap closure yet. |

### Human Verification Required

### 1. First-click keyboard ownership

**Test:** Run `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release`, confirm mdCAD does not auto-focus on attach, click once inside mdCAD, press `Tab`, then click a host control/status area.
**Expected:** The first click grants mdCAD keyboard ownership, `Tab` stays inside mdCAD, and clicking host UI returns focus to the host.
**Why human:** Cross-process Win32 focus ownership and keyboard routing are not covered by the existing reducer/unit tests.

### 2. Destroyed-parent self-exit

**Test:** Run `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release -- --embed-test-mode destroyed-parent`.
**Expected:** Status reaches `teardown/cleanup` with destroyed-parent mode, reports mdCAD self-exit rather than host fallback cleanup, shows no save prompt/standalone fallback, and leaves no surviving `mdCAD.exe`.
**Why human:** The host wait path and mdCAD invalid-parent quit path are present in code, but the real exit race is an end-to-end process/window lifecycle behavior.

### 3. Embedded layout persistence and standalone isolation

**Test:** Remove `build-vulkan\bin\Release\imgui.embedded.ini`, launch the host, verify the first-run viewer-first layout, rearrange panels, close the host normally, relaunch embedded mdCAD, then launch standalone `build-vulkan\bin\Release\mdCAD.exe`.
**Expected:** Embedded layout persists via `build-vulkan\bin\Release\imgui.embedded.ini`, and standalone mdCAD continues using `imgui.ini` without inheriting embedded layout changes.
**Why human:** Persistence/isolation correctness depends on visual docking state across multiple launches and modes.

### Gaps Summary

No automated code gap remains obvious in the 44-05/44-06 change set: the reducer-backed shortcut gate, graceful teardown wait path, explicit embedded-store path resolution, and persistence hooks are all present and build/test cleanly. Phase 44 still cannot be marked `passed`, however, because the current UAT/checklist evidence is stale and the previously failing interactive scenarios (2, 5, and 7) have not been re-run after the fixes.

---

_Verified: 2026-05-14T20:03:09.9504849Z_
_Verifier: the agent (gsd-verifier)_
