---
phase: 9
slug: long-tail-validation-performance-gates-and-boundary-finalization
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 9 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | In-repo native harness (`mdcad_math_harness`) + CMake custom targets |
| **Config file** | `src/CMakeLists.txt` target wiring |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdcad_math_harness && .\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict` |
| **Full suite command** | `cmake --build build-vulkan --config Release --target math-validation` |
| **Estimated runtime** | ~120 seconds (host-dependent, excludes manual smoke) |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdcad_math_harness && .\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict`
- **After every plan wave:** Run `cmake --build build-vulkan --config Release --target math-validation`
- **Before `/gsd-verify-work`:** Full suite must be green and all Phase 9 evidence artifacts present
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 09-01-01 | 09-01 | 1 | VAL-01 | compare coverage audit | `.\build-vulkan\bin\Release\mdcad_math_harness.exe --list` + strict compare output capture | ❌ W0 | ⬜ pending |
| 09-01-02 | 09-01 | 1 | VAL-01 | regression compare | `.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode compare --strict` | ✅ | ⬜ pending |
| 09-02-01 | 09-02 | 2 | VAL-02 | benchmark gate | `.\build-vulkan\bin\Release\mdcad_math_harness.exe --mode bench --iterations 2000000` + `py -3 scripts\eval_math_bench.py ...` | ✅ | ⬜ pending |
| 09-03-01 | 09-03 | 3 | VAL-03 | manual integration checklist | `.\build-vulkan\bin\Release\mdCAD.exe` with checklist/report capture | ❌ W0 | ⬜ pending |
| 09-03-02 | 09-03 | 3 | TRED-02 | static boundary audit | `rg "ray_from_screen\(|ray_axis_closest_t\(|ray_plane_intersect\(" src` + boundary doc update | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠ flaky*

---

## Wave 0 Requirements

- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/coverage/harness-coverage-map.md` — maps long-tail requirements/touchpoints to compare cases and manual checks
- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-checklist.md` — integrated serializer/importer/undo/editor checklist
- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-report.md` — integrated manual pass/fail evidence
- [ ] `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/boundary/thin-entrypoint-boundary-finalization.md` — retained-vs-removable thin-entrypoint contract with consumer traceability

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Save/load round-trip parity in interactive app workflows | VAL-03 | Requires user-facing interaction and scene inspection | Launch `mdCAD`, run Phase 6-style serializer checks, record outcomes in Phase 9 manual report |
| JSONL/PLY import placement/orientation/scale confidence | VAL-03 | Runtime scene semantics require visual and workflow confirmation | Execute import workflows from Phase 7 checklist patterns, log parity fields and deltas |
| Undo/redo + inspector + gizmo edit confidence | VAL-03 | Interaction and editor behavior are workflow-level, not unit-only | Execute Phase 8 mandatory flows and record result in integrated long-tail manual report |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all missing artifact references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
