---
phase: 02
slug: direct-adoption-tooling-and-validation-harness
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-24
---

# Phase 02 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | other |
| **Config file** | none — Phase 2 uses custom CMake targets and a standalone native harness |
| **Quick run command** | `cmake -B build -G Ninja && ninja -C build mdcad_math_harness` |
| **Full suite command** | `cmake -B build -G Ninja && ninja -C build math-validation` |
| **Estimated runtime** | ~40 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake -B build -G Ninja && ninja -C build mdcad_math_harness`
- **After every plan wave:** Run `cmake -B build -G Ninja && ninja -C build math-validation`
- **Before `$gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 45 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 01 | 1 | FOUND-02 | static | `rg -n "CGLM_FORCE_DEPTH_ZERO_TO_ONE|CGLM_CONFIG_CLIP_CONTROL == CGLM_CLIP_CONTROL_RH_ZO|_Static_assert\\(sizeof\\(mat4\\)" src/math/cglm_entry.h` | ✅ | ⬜ pending |
| 02-01-02 | 01 | 1 | FOUND-02 | static | `rg -n "mdcad_compare_mat4_close|mdcad_validation_expect|mdcad_bench_run|timespec_get" src/math/math_compare.h src/math/math_validate.h src/math/math_bench.h` | ✅ | ⬜ pending |
| 02-02-01 | 02 | 2 | FOUND-02 | build | `cmake -B build -G Ninja && ninja -C build mdcad_math_harness` | ✅ | ⬜ pending |
| 02-02-02 | 02 | 2 | PERF-01 | runtime | `./build/bin/mdcad_math_harness --mode compare --strict && ./build/bin/mdcad_math_harness --mode bench --iterations 1000 | rg -n "^BENCH (legacy-mat4-mul|cglm-mat4-mul|legacy-mat4-inverse|cglm-mat4-inv|legacy-screen-ray|cglm-screen-ray)"` | ✅ | ⬜ pending |
| 02-03-01 | 03 | 3 | PERF-01 | build | `cmake -B build -G Ninja && ninja -C build math-regression math-bench math-validation` | ✅ | ⬜ pending |
| 02-03-02 | 03 | 3 | FOUND-02, PERF-01 | static | `rg -n "Native Math Validation \\(Phase 2 pilot\\)|mdcad_math_harness|math-regression|math-bench|math-validation|secondary" docs/QUICKSTART.md src/CMakeLists.txt` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Optional native app smoke after harness run | PERF-01 | Harness is primary, but a manual app launch can still catch obvious startup regressions | After `ninja -C build math-validation`, run `./build/bin/mdCAD`, orbit the camera briefly, and confirm the app still launches cleanly |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 45s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
