---
phase: 43
slug: embed-contract-child-window-bootstrap
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-14
---

# Phase 43 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 43-W0-01 | TBD | 0 | EMBD-01, EMBD-03 | unit | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test` | ❌ W0 | ⬜ pending |
| 43-W0-02 | TBD | 0 | EMBD-02 | manual smoke | `dotnet build samples/avalonia-host -c Release` | ❌ W0 | ⬜ pending |
| 43-W0-03 | TBD | 0 | HOST-01 | build smoke + manual attach smoke | `dotnet build samples/avalonia-host -c Release` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/embed_launch_config_test.c` — parser coverage for `--embedded` and `--parent-hwnd`
- [ ] `samples/avalonia-host/*.csproj` — sample host build target for `HOST-01`
- [ ] Embedded-startup manual smoke checklist — attach/failure path coverage for true child-window launch
- [ ] Pinned Sokol revision / patch-hosting decision before Win32 embed implementation starts

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| mdCAD appears only as a true child window inside the sample host | EMBD-02, HOST-01 | Current test infrastructure does not automate cross-process native-child attachment | Build and run the Avalonia sample, confirm mdCAD appears only inside the host region, and verify no standalone top-level mdCAD window is shown |
| Invalid embedded launch fails without fallback | EMBD-03 | Requires end-to-end process + window-behavior observation | Launch mdCAD with bad `--parent-hwnd` input from the sample/bootstrap harness and confirm explicit failure with no standalone fallback |
| Embedded bootstrap applies requested debug-window defaults | HOST-01 | UI visibility state is runtime-visual, not currently covered by tests | Launch embedded mdCAD and confirm `Pick Buffer Debug`, `Slot Buffer Debug`, and `FPS Debug` are OFF by default |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
