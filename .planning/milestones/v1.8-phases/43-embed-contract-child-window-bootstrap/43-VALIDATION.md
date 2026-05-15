---
phase: 43
slug: embed-contract-child-window-bootstrap
status: draft
nyquist_compliant: true
wave_0_complete: true
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
- **After every task that touches `samples/avalonia-host/`:** Run `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Early-Feedback Strategy

Wave 0 is intentionally front-loaded in Plan `43-01` instead of waiting until host closeout:

- `43-01 / Task 1` pins Sokol and the repo-hosted patch seam.
- `43-01 / Task 2` creates the fast parser CTest for EMBD-01 / EMBD-03.
- `43-01 / Task 3` creates a buildable Avalonia sample-host scaffold plus the valid/invalid/destroyed parent-HWND smoke checklist shell.

That means the phase has no unresolved `MISSING` verification dependency before Win32 embed wiring begins, so `nyquist_compliant: true` and `wave_0_complete: true` are correct at planning time. Later plans finish the child-window path and then execute the host smoke proof against that already-built harness.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 43-01-T1 | 43-01 | 0 | EMBD-02 | config/static | `python -c "from pathlib import Path; t=Path('vendors/libsokol/CMakeLists.txt').read_text(encoding='utf-8'); assert '2356d22badb8b02b5b4fc745216023a2a699d840' in t; assert '0001-win32-embed-child-window-bootstrap.patch' in t; assert '${CMAKE_SOURCE_DIR}/src' in t"` | ✅ planned | ⬜ pending |
| 43-01-T2 | 43-01 | 0 | EMBD-01, EMBD-03 | unit | `ctest --test-dir build-vulkan -C Release --output-on-failure -R embed_launch_config_test` | ✅ planned | ⬜ pending |
| 43-01-T3 | 43-01 | 0 | HOST-01, EMBD-03 | build smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ planned | ⬜ pending |
| 43-02-T1 | 43-02 | 1 | EMBD-02 | config/static | `python -c "from pathlib import Path; patch=Path('vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch').read_text(encoding='utf-8'); helper=Path('src/platform/win32_embed.h').read_text(encoding='utf-8'); assert 'WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN' in patch; assert 'GetClientRect' in patch; assert 'SetParent' not in patch; assert 'mdcad_win32_embed_fail_startup' in helper"` | ✅ planned | ⬜ pending |
| 43-02-T2 | 43-02 | 1 | EMBD-02, EMBD-03 | native build | `cmake --build build-vulkan --config Release --target mdCAD` | ✅ planned | ⬜ pending |
| 43-03-T1 | 43-03 | 2 | HOST-01, EMBD-03 | build smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ planned | ⬜ pending |
| 43-03-T2 | 43-03 | 2 | HOST-01, EMBD-03 | checklist/static | `python -c "from pathlib import Path; t=Path('.planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md').read_text(encoding='utf-8'); required=['Scenario 1 - valid attach','Scenario 2 - invalid parent HWND failure','Scenario 3 - destroyed parent HWND failure','Observed states:','Embed failure evidence:','Child window result:','Standalone fallback result:','Debug-window baseline:','Scenario verdict:','Checklist verdict:']; assert all(x in t for x in required); assert t.count('TBD') >= 9"` | ✅ planned | ⬜ pending |
| 43-03-T3 | 43-03 | 2 | HOST-01, EMBD-03 | manual checkpoint + automated completion check | `python -c "from pathlib import Path; t=Path('.planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md').read_text(encoding='utf-8'); assert 'TBD' not in t; assert ('Checklist verdict: PASS' in t) or ('Checklist verdict: FAIL' in t)"` | ✅ planned | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/tests/embed_launch_config_test.c` — parser coverage for `--embedded` and `--parent-hwnd` (`43-01 / Task 2`)
- [x] `samples/avalonia-host/*.csproj` — sample host build target for `HOST-01` (`43-01 / Task 3`)
- [x] Embedded-startup manual smoke checklist — attach/failure path coverage for true child-window launch, including invalid/destroyed parent-HWND scenarios (`43-01 / Task 3`, completed in `43-03 / Tasks 2-3`)
- [x] Pinned Sokol revision / patch-hosting decision before Win32 embed implementation starts (`43-01 / Task 1`)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| mdCAD appears only as a true child window inside the sample host | EMBD-02, HOST-01 | Current test infrastructure does not automate cross-process native-child attachment | Build and run the Avalonia sample, confirm mdCAD appears only inside the host region, and verify no standalone top-level mdCAD window is shown |
| Invalid or destroyed parent-HWND launch fails without fallback | EMBD-03 | Requires end-to-end process + window-behavior observation across host + child process | Launch mdCAD from the sample-host harness with `--embed-test-mode invalid-parent` and `--embed-test-mode destroyed-parent`, confirm the host surfaces `Embedded startup failed:`, and verify no standalone fallback window appears |
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
