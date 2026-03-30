---
phase: 10
slug: sketch-foundations-managers
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-30
---

# Phase 10 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Build-gate + manual UI smoke (no dedicated unit-test framework detected) |
| **Config file** | none — existing validation is command/checklist based |
| **Quick run command** | cmake --build build-vulkan --config Release --target mdCAD |
| **Full suite command** | cmake --build build-vulkan --config Release --target math-validation |
| **Estimated runtime** | environment-dependent |

---

## Sampling Rate

- **After every task commit:** Run cmake --build build-vulkan --config Release --target mdCAD
- **After every plan wave:** Run cmake --build build-vulkan --config Release --target math-validation
- **Before /gsd-verify-work:** Full suite must be green
- **Max feedback latency:** environment-dependent

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 10-XX-YY | TBD | TBD | SKCH-01 | smoke/manual | cmake --build build-vulkan --config Release --target mdCAD + in-app checklist | ✅ | ⬜ pending |
| 10-XX-YY | TBD | TBD | SKCH-02 | smoke/manual | cmake --build build-vulkan --config Release --target mdCAD + in-app checklist | ✅ | ⬜ pending |
| 10-XX-YY | TBD | TBD | SKCH-03 | smoke/manual | cmake --build build-vulkan --config Release --target mdCAD + in-app checklist | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] Add phase-specific manual smoke checklist artifact under .planning/phases/10-sketch-foundations-managers/evidence/
- [ ] Add automated harness/CLI smoke hooks for sketch manager operations (currently manual-only)
- [ ] Add undo behavior check for multi-select atomicity

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Sketch creation and geometry attachment UX correctness | SKCH-01 | Requires interactive UI flows in viewport/panels | Run app, create sketch via Add Entity, add point/line/arc via Add Entity and GeometryManager, verify entities parent under sketch |
| Inspector status/policy/count surfaces | SKCH-02 | Requires visual verification of Inspector sections | Select sketch entity and verify status label, color policy text, geometry count, and constraint count values update after edits |
| GeometryManager multi-select action + one-step undo semantics | SKCH-03 | Undo stack behavior is interaction-driven | Multi-select geometries, run fix/unfix/delete, then undo once and verify all affected entities revert together |

---

## Validation Sign-Off

- [ ] All tasks have <automated> verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all missing references
- [ ] No watch-mode flags
- [ ] 
yquist_compliant: true set in frontmatter

**Approval:** pending
