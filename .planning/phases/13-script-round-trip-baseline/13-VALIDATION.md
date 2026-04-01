---
phase: 13
slug: script-round-trip-baseline
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-01
---

# Phase 13 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake/CTest targets (project-level), plus command smoke validation |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdcad` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdcad`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 13-01-01 | 01 | 1 | SCRP-01 | integration/UI smoke | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 13-01-02 | 01 | 1 | SCRP-06 | smoke/env check | `lua -v` | ❌ W0 | ⬜ pending |
| 13-02-01 | 02 | 2 | SCRP-02 | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 13-02-02 | 02 | 2 | SCRP-03 | unit + integration | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] Add dedicated script round-trip tests (parser/apply/emitter determinism) under source-controlled test target.
- [ ] Add Lua runtime/version assertion test or startup guard for SCRP-06.
- [ ] Add deterministic golden-file style script emit checks for no-op edit stability.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Script editor open/close flow from SketchManager | SCRP-01 | Interactive standalone window behavior and persistence need viewport/UI exercise | Open sketch in Entity Inspector, launch Script Editor, move/dock window, restart app, confirm window visibility and state behavior remain correct |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
