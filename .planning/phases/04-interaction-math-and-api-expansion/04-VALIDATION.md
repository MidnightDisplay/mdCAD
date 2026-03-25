---
phase: 04
slug: interaction-math-and-api-expansion
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-25
---

# Phase 04 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake/Ninja + `mdcad_math_harness` compare/bench gates |
| **Config file** | `src/CMakeLists.txt` (targets), `src/math_harness.c` (cases) |
| **Quick run command** | `./build/bin/mdcad_math_harness --mode compare --strict` |
| **Full suite command** | `cmake -B build -G Ninja && ninja -C build math-validation` |
| **Estimated runtime** | ~30-90 seconds |

---

## Sampling Rate

- **After every task commit:** Run `./build/bin/mdcad_math_harness --mode compare --strict`
- **After every plan wave:** Run `cmake -B build -G Ninja && ninja -C build math-validation`
- **Before `$gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 04-00-01 | TBD | TBD | HOT-03 | compare+harness | `./build/bin/mdcad_math_harness --mode compare --strict` | ✅ | ⬜ pending |
| 04-00-02 | TBD | TBD | EXP-01 | compare + unit-style harness case | `./build/bin/mdcad_math_harness --mode compare --strict` | ✅ | ⬜ pending |
| 04-00-03 | TBD | TBD | EXP-02 | static usage audit + compare | `rg "ray_from_screen|ray_axis_closest_t|ray_plane_intersect" src/app.c src/gpu/pick_buffer.h src/gizmo/*.h` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] Add/extend harness compare cases for pick-MVP and gizmo drag parity in `src/math_harness.c`
- [ ] Add/extend harness bench cases for migrated interaction helpers in `src/math_harness.c`
- [ ] Ensure build target wiring remains available via `math-validation`

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Hover/click selection fidelity near dense geometry | HOT-03 | Depends on live viewport interaction feel | Launch `./build/bin/mdCAD`, test hover and click accuracy around thin lines/points and crowded geometry |
| Axis drag starts without jump and remains stable | HOT-03 | Feel-sensitive at drag start | Select entity, drag each gizmo axis from multiple camera angles, confirm no initial snap/jitter |
| Plane drag behavior across camera orientations | HOT-03 | User-perceived manipulation behavior | Drag XY/XZ/YZ handles while orbiting camera, confirm consistent delta behavior |
| Geometry-mode vertex drag correctness under rotated/scaled transforms | HOT-03 | Hard to fully capture with automated visual parity alone | Use geometry mode on transformed entity, drag vertices, verify expected local-space edits |
| Undo/redo correctness after transform and vertex drags | HOT-03 | Interaction sequence statefulness | Perform drag operations, run undo/redo cycles, verify positions/vertices restore exactly |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
