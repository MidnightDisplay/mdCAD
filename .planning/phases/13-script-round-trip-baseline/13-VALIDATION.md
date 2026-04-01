---
phase: 13
slug: script-round-trip-baseline
status: draft
nyquist_compliant: true
wave_0_complete: true
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
| **Quick run command** | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` |
| **Full suite command** | `cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` |
| **Estimated runtime** | smoke <60s target; full gate ~60-180s |

---

## Sampling Rate

- **After every task commit:** Run fast smoke `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` (target <60s).
- **After every plan wave:** Run full gate for changed plan target (`script_roundtrip_tests` for 13-01/13-02, `mdCAD` + `script_roundtrip_tests` for 13-03).
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 13-01-01 | 01 | 1 | SCRP-02 | integration + persistence | `Fast: ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`<br>`Full: cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ | ⬜ pending |
| 13-01-02 | 01 | 1 | SCRP-06 | runtime/env baseline | `Fast: lua -v && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`<br>`Full: cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests && lua -v` | ✅ | ⬜ pending |
| 13-02-01 | 02 | 2 | SCRP-02 | parser/apply integration | `Fast: ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`<br>`Full: cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ | ⬜ pending |
| 13-02-02 | 02 | 2 | SCRP-03 | deterministic emit integration | `Fast: ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`<br>`Full: cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ | ⬜ pending |
| 13-03-01 | 03 | 3 | SCRP-01, SCRP-03 | UI integration + editor loop | `Fast: ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`<br>`Full: cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ | ⬜ pending |
| 13-03-02 | 03 | 3 | SCRP-01 | checkpoint:human-verify | `Fast: ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`<br>`Full: cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- ✅ Dedicated script round-trip tests are planned via `src/tests/script_roundtrip_tests.c` and `add_test(NAME script_roundtrip_tests ...)` in `src/CMakeLists.txt` (13-01).
- ✅ Lua runtime/version assertion baseline is covered in runtime helpers and tests (`lua -v` + scripted runtime checks) per 13-01.
- ✅ Deterministic emit/no-op stability checks are explicitly planned in 13-02 tests.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Script editor open/close flow from SketchManager | SCRP-01 | Interactive standalone window behavior and persistence need viewport/UI exercise | Open sketch in Entity Inspector, launch Script Editor, move/dock window, restart app, confirm window visibility and state behavior remain correct |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all previously MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
