---
phase: 11
slug: constraint-authoring-ux
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-31
---

# Phase 11 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake/CTest harness + manual UI checklist |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdCAD` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdCAD`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 11-00-01 | 01 | 0 | SKCH-04 | manual UI smoke | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |
| 11-00-02 | 01 | 0 | CONS-01 | manual UI matrix | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |
| 11-00-03 | 01 | 0 | CONS-02 | manual interaction | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |
| 11-00-04 | 01 | 0 | CONS-03 | manual viewport | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |
| 11-00-05 | 01 | 0 | CONS-04 | manual popup+manager | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |
| 11-00-06 | 01 | 0 | CONS-05 | manual driven-state | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `.planning/phases/11-constraint-authoring-ux/11-VALIDATION.md` requirement map maintained as plan tasks are finalized
- [ ] Add/extend lightweight automated smoke check for key routing + pick ID range collision guard
- [ ] Add regression checks for undo/redo around constraint create/delete/value edits where practical

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Select constraint highlights all participants | SKCH-04 | Depends on live viewport selection/highlight rendering | Build app, create sketch geometry, apply constraint, select row/glyph, confirm all participants highlight immediately |
| In-context applicable constraints and one-shot apply | CONS-01, CONS-02 | Requires dynamic legality filtering from active selection | Select valid/invalid geometry combinations, press `C`, verify only legal options appear, apply one option, verify menu closes |
| Glyph hover/select from constant-size overlays | CONS-03 | Requires visual interaction at different zoom levels | Zoom in/out and pan, hover/select constraint glyphs, confirm stable constant-size interaction and correct target selection |
| LENGTH/ANGLE mirrored editing | CONS-04 | Popup + manager mirrored state is interactive UX | Double-click dimension glyph, edit value, accept/cancel, verify manager row mirrors value and vice versa |
| Driven toggle visibility/persistence | CONS-05 | No solver semantics in phase; UI/state persistence check | Mark dimension as driven in manager/popup, save/reload scene if supported, confirm driven flag persists and displays |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or explicit Wave 0/manual dependency
- [ ] Sampling continuity: no 3 consecutive tasks without at least quick build verification
- [ ] Wave 0 captures all missing automated checks identified in research
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [ ] `nyquist_compliant: true` set in frontmatter once validation contract is fully satisfied

**Approval:** pending
