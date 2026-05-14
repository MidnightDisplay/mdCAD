---
phase: 44
slug: embedded-resize-focus-viewer-layout
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-14
---

# Phase 44 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C unit tests; manual Avalonia/Win32 smoke for UI lifecycle |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt`, generated `build-vulkan/CTestTestfile.cmake` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdCAD embed_input_state_test embed_layout_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_launch_config_test|embed_input_state_test|embed_layout_state_test"` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~60-90 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdCAD embed_input_state_test embed_layout_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_launch_config_test|embed_input_state_test|embed_layout_state_test"`
- **After every task that touches `samples/avalonia-host/`:** Run `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure` plus the current manual embedded smoke checklist
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Early-Feedback Strategy

Wave 0 is intentionally front-loaded in `44-01` before runtime focus/layout/lifecycle edits land:

- `44-01 / Task 1` creates the native reducer/unit-test seams for embedded input-state and embedded layout-state rules.
- `44-01 / Task 2` creates the Phase 44 manual checklist shell covering resize, first-click focus, click-away return, Alt+Tab cancel, host close, destroyed parent, and the dedicated `destroy-after-attach` harness mode.
- `44-02` then wires the mdCAD-side focus/capture and invalid-parent `sapp_quit()` path against those seams.
- `44-03` finishes the Avalonia harness and binds the manual checklist to the real commands and observations.

That means the phase has no unresolved `MISSING` verification dependency before the deeper runtime work begins, so `nyquist_compliant: true` and `wave_0_complete: true` are correct for this plan set.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 44-01-T1 | 44-01 | 1 | INPT-02, INPT-03, INPT-04 | unit | `cmake --build build-vulkan --config Release --target embed_input_state_test embed_layout_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_input_state_test|embed_layout_state_test"` | ✅ planned | ⬜ pending |
| 44-01-T2 | 44-01 | 1 | EMBD-04, INPT-01, INPT-02, INPT-03, INPT-04 | checklist/static | `python -c "from pathlib import Path; p=Path(r'.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md'); t=p.read_text(encoding='utf-8'); required=['Scenario 1','resize','first-click','Alt+Tab','destroy']; missing=[x for x in required if x not in t]; assert not missing, missing; print('checklist-shell-ok')"` | ✅ planned | ⬜ pending |
| 44-02-T1 | 44-02 | 2 | EMBD-04, INPT-02, INPT-03 | native build + unit regression | `cmake -S . -B build-vulkan -DUSE_VULKAN=ON && cmake --build build-vulkan --config Release --target mdCAD embed_input_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_launch_config_test|embed_input_state_test"` | ✅ planned | ⬜ pending |
| 44-02-T2 | 44-02 | 2 | INPT-03 | native build + unit regression | `cmake --build build-vulkan --config Release --target mdCAD embed_input_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_input_state_test` | ✅ planned | ⬜ pending |
| 44-03-T1 | 44-03 | 2 | EMBD-04, INPT-01 | build smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ planned | ⬜ pending |
| 44-03-T2 | 44-03 | 2 | EMBD-04, INPT-01 | checklist/static | `python -c "from pathlib import Path; t=Path(r'.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md').read_text(encoding='utf-8'); required=['destroy-after-attach','no standalone fallback','no surviving mdCAD.exe','snap']; missing=[x for x in required if x not in t]; assert not missing, missing; print('manual-checklist-bound-ok')"` | ✅ planned | ⬜ pending |
| 44-04-T1 | 44-04 | 3 | INPT-04 | unit | `cmake --build build-vulkan --config Release --target mdCAD embed_layout_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_layout_state_test` | ✅ planned | ⬜ pending |
| 44-04-T2 | 44-04 | 3 | INPT-04 | unit | `cmake --build build-vulkan --config Release --target mdCAD embed_layout_state_test && ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_layout_state_test` | ✅ planned | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/tests/embed_input_state_test.c` — focus-state reducer coverage for first-click ownership and cancel/reset decisions before `44-02` runtime wiring (`44-01 / Task 1`)
- [x] `src/tests/embed_layout_state_test.c` — embedded-vs-standalone layout storage/selection coverage for `INPT-04` before `44-04` dock/persistence edits (`44-01 / Task 1`)
- [x] `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` — resize/focus/Alt+Tab/orphan manual evidence shell for `EMBD-04`, `INPT-01..04`, later bound to the real host harness in `44-03 / Task 2`
- [x] Deterministic host harness hook for destroying the placeholder without closing the host window so `EMBD-04` can be reproed deterministically (`44-03 / Task 1`)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Host resize keeps the embedded child surface and viewport coherent during drag, snap-layout, and restore/maximize transitions | INPT-01 | Current test infrastructure does not automate cross-process HWND resize + viewport observation | Run the Avalonia host, resize aggressively, use snap/fullscreen transitions, and confirm mdCAD stays visually correct with no clipped or stale viewport state |
| First click activates mdCAD without launch-time focus theft, and clicking host UI returns focus cleanly | INPT-02 | Cross-process Win32 focus ownership and click consumption require live interaction | Launch embedded mdCAD, confirm it does not auto-focus on attach, click inside once to activate, then click host chrome/status region to confirm focus returns cleanly |
| Alt+Tab / host deactivation clears active capture and drag state without leaving stuck interactions | INPT-03 | Requires OS-level activation changes across host and child windows | Start a camera/gizmo drag, Alt+Tab or click away to another app, then return and confirm no stuck drag/capture remains |
| Embedded default layout fits the hosted region and persists separately from standalone layout | INPT-04 | Requires visual docking confirmation across multiple launches/modes | Verify the approved embedded dock layout on first run, rearrange panels, relaunch embedded mdCAD, and confirm embedded persistence works without disturbing standalone layout |
| Host close or parent invalidation exits embedded mdCAD with no standalone fallback | EMBD-04 | `44-03-T1` automates host-harness build coverage, but proving mdCAD-side invalid-parent `sapp_quit()` behavior versus host-side `Process.Kill(true)` cleanup still requires end-to-end process/window observation | Run the checklist scenarios for `destroyed-parent`, host close, and `destroy-after-attach`; confirm the parent-ended path exits mdCAD without a save prompt or standalone fallback, and the fallback host-kill path also leaves no surviving `mdCAD.exe` |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
