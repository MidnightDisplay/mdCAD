---
phase: 12
slug: solver-control-and-constrained-interaction
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-01
---

# Phase 12 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake build gate + CTest harness + manual interactive verification |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdCAD` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~60-180 seconds (machine dependent) |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdCAD`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Run `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Max feedback latency:** under one local edit cycle (single build cycle target)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 12-01-01 | 01 | 1 | SOLV-01 | targeted smoke + manual + gate | `grep -n "auto_solve_enabled\|scene_solver_\|INFO\|WARNING\|ERROR" src/components/sketch_comp.h src/ecs/ecs_scene.h` | ✅ | ⬜ pending |
| 12-01-02 | 01 | 1 | SOLV-02 | targeted smoke + manual + gate | `grep -n "Recalculate Sketch\|Clear Diagnostics History\|INFO\|WARNING\|ERROR\|backend" src/ui/ui_entity_inspector.h` | ✅ | ⬜ pending |
| 12-02-01 | 02 | 2 | SOLV-03 | targeted smoke + manual + gate | `grep -n "implicat\|scene_solver_can_apply_drag\|projected\|first\|success" src/ecs/ecs_scene.h` | ✅ | ⬜ pending |
| 12-02-02 | 02 | 2 | SOLV-04, API-03 | targeted smoke + manual + gate | `grep -n "Movement blocked by active constraints\.\|Drag rejected: active constraints make this move invalid\.\|constraint_selection_apply_participants\|scene_solver_can_apply_drag" src/app.c` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `12-VALIDATION.md` exists and maps `SOLV-01..04` + `API-03` to explicit verification rows.
- [x] Manual-only rows include companion automated gate command.
- [x] Existing infrastructure accepted as the phase baseline (no new test framework required for planning).

*Existing infrastructure covers all phase requirements at planning time; execution may still add targeted checks if warranted.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Auto-solve toggle and manual recalc interaction in SketchManager | SOLV-01 | UI interaction flow and per-sketch state are runtime behaviors | In active sketch, toggle auto-solve, perform geometry/constraint edits, confirm solve trigger behavior matches selected mode; press Recalculate and confirm explicit solve path. |
| Status and diagnostics rendering with timestamp and severity labels | SOLV-02 | Requires live UI rendering and event ordering observation | Trigger solved/loose/fixed/error transitions and confirm status text + timestamped `INFO/WARNING/ERROR` diagnostics entries update as specified. |
| Single backend visibility in solver controls | SOLV-03 | Presentation/contract check in UI | Confirm solver panel displays one active backend identity and no runtime backend-switch path is exposed for v1.2. |
| Failure implication highlighting and focus behavior | SOLV-04 | Requires viewport + selection behavior parity checks | Trigger failing solve; verify implicated constraint is focused and participant geometry highlight persists until next successful solve. |
| Constrained gizmo interaction under active constraints | API-03 | Real-time drag projection/blocking behavior is interactive | Drag constrained geometry in valid and unsatisfiable directions; verify projected motion when feasible and blocked motion with immediate feedback/log on unsatisfiable edits. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` companion command or explicit manual+gate mapping
- [x] Sampling continuity defined at task/wave/phase gate levels
- [x] Wave 0 covers all missing validation references for this phase
- [x] No watch-mode flags
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-04-01

