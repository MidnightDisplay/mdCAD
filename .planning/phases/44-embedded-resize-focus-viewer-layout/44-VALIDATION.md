---
phase: 44
slug: embedded-resize-focus-viewer-layout
status: draft
nyquist_compliant: false
wave_0_complete: false
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
| **Quick run command** | `ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_launch_config_test|embed_.*_test"` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure -R "embed_launch_config_test|embed_.*_test"`
- **After every task that touches `samples/avalonia-host/`:** Run `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure` plus the current manual embedded smoke checklist
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Early-Feedback Strategy

Phase 44 starts with missing Wave 0 coverage, so planning must front-load the validation scaffolding before deeper focus/layout changes:

- Add a native reducer/unit-test seam for embedded input-state cancellation before changing Sokol/app drag behavior.
- Add an embedded-layout persistence seam and unit test before dock/persistence changes spread through `app.c`.
- Add a Phase 44 manual checklist/harness coverage for resize, first-click focus, click-away return, Alt+Tab cancel, and host-close/orphan checks before final verification.

Because these Wave 0 dependencies are still missing, `nyquist_compliant: false` and `wave_0_complete: false` are correct at planning time.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 44-W0-T1 | TBD | 0 | INPT-02, INPT-03 | unit | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_input_state_test` | ❌ Wave 0 | ⬜ pending |
| 44-W0-T2 | TBD | 0 | INPT-04 | unit | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_layout_state_test` | ❌ Wave 0 | ⬜ pending |
| 44-W0-T3 | TBD | 0 | EMBD-04, INPT-01, INPT-02, INPT-03, INPT-04 | build smoke + manual checklist harness | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/embed_input_state_test.c` — focus-state reducer, orbit-camera cancel, and gizmo cancel/rollback coverage for `INPT-02` / `INPT-03`
- [ ] `src/tests/embed_layout_state_test.c` — embedded-vs-standalone layout storage/selection coverage for `INPT-04`
- [ ] `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` — resize/focus/Alt+Tab/orphan manual evidence for `EMBD-04`, `INPT-01..04`
- [ ] Optional host harness hook for destroying the placeholder without closing the host window so `EMBD-04` can be reproed deterministically

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Host resize keeps the embedded child surface and viewport coherent during drag, snap-layout, and restore/maximize transitions | INPT-01 | Current test infrastructure does not automate cross-process HWND resize + viewport observation | Run the Avalonia host, resize aggressively, use snap/fullscreen transitions, and confirm mdCAD stays visually correct with no clipped or stale viewport state |
| First click activates mdCAD without launch-time focus theft, and clicking host UI returns focus cleanly | INPT-02 | Cross-process Win32 focus ownership and click consumption require live interaction | Launch embedded mdCAD, confirm it does not auto-focus on attach, click inside once to activate, then click host chrome/status region to confirm focus returns cleanly |
| Alt+Tab / host deactivation clears active capture and drag state without leaving stuck interactions | INPT-03 | Requires OS-level activation changes across host and child windows | Start a camera/gizmo drag, Alt+Tab or click away to another app, then return and confirm no stuck drag/capture remains |
| Embedded default layout fits the hosted region and persists separately from standalone layout | INPT-04 | Requires visual docking confirmation across multiple launches/modes | Verify the approved embedded dock layout on first run, rearrange panels, relaunch embedded mdCAD, and confirm embedded persistence works without disturbing standalone layout |
| Host close or parent invalidation exits embedded mdCAD with no standalone fallback | EMBD-04 | Requires end-to-end process/window lifecycle observation | Close the host and exercise parent-invalid teardown paths; confirm `mdCAD.exe` does not survive and no standalone fallback window appears |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
