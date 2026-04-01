---
phase: 16
slug: constraint-ux-closure-and-verification
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-01
---

# Phase 16 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CMake/CTest + manual UI checklist |
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
| 16-01-01 | 16-01 | 1 | SKCH-04 | build + manual UI | `cmake --build build-vulkan --config Release --target mdCAD` | ✅ | ⬜ pending |
| 16-01-02 | 16-01 | 1 | CONS-03 | build + manual viewport | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 16-02-01 | 16-02 | 2 | CONS-01, CONS-02, CONS-04, CONS-05 | docs + build evidence | `cmake --build build-vulkan --config Release --target mdCAD` | ✅ | ⬜ pending |
| 16-02-02 | 16-02 | 2 | SKCH-04, CONS-01..05 | docs compliance gate | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `.planning/phases/16-constraint-ux-closure-and-verification/16-VALIDATION.md` exists for Nyquist Dimension 8 gate
- [ ] Add execution evidence rows/results after implementing `16-01-PLAN.md`
- [ ] Add `11-VERIFICATION.md` evidence links after implementing `16-02-PLAN.md`

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Glyph click highlights all participants immediately | SKCH-04, CONS-03 | Requires live viewport selection rendering | Build app, create sketch + constraints, click glyph, confirm all alive participants are highlighted on same frame |
| Manager row select and glyph click produce same participant set | SKCH-04 | Requires interactive comparison across entrypoints | Select same constraint via manager row then glyph, verify identical highlighted entities |
| Context menu applicability and auto-hide | CONS-01, CONS-02 | Dynamic legality filtering and one-shot behavior is interaction-driven | Select valid/invalid geometry sets, press `C`, verify only legal constraints show, apply one, verify menu closes |
| Dimensional popup mirror and close paths | CONS-04 | Popup + manager mirrored values require UI interaction | Double-click LENGTH/ANGLE glyph, edit via `Apply Value`/Enter, verify manager mirrors; verify `Discard Changes`/Escape/outside click cancels |
| Driven toggle visibility/readability behavior | CONS-05 | Driven semantics are UI/state flow checks | Toggle Driven in manager and selected-constraint surfaces, verify consistent display and persisted state on save/load |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or explicit manual dependency with command companion
- [ ] Sampling continuity: no 3 consecutive tasks without automated verification
- [ ] Wave 0 gates completed for execution start
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [ ] `nyquist_compliant: true` set in frontmatter after execution evidence is green

**Approval:** pending
