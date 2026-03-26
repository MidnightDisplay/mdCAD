---
phase: 06
slug: serializer-and-save-load-long-tail-migration
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-26
---

# Phase 06 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Native CMake targets + targeted serializer integration checks |
| **Config file** | src/CMakeLists.txt |
| **Quick run command** | cmake --build build-vulkan --config Release --target mdcad_math_harness |
| **Full suite command** | cmake --build build-vulkan --config Release --target math-validation |
| **Estimated runtime** | ~60-180 seconds |

---

## Sampling Rate

- **After every task commit:** Run cmake --build build-vulkan --config Release --target mdcad_math_harness
- **After every plan wave:** Run cmake --build build-vulkan --config Release --target math-validation
- **Before /gsd-verify-work:** Full suite must be green
- **Max feedback latency:** 300 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 06-01-01 | 01 | 1 | TAIL-01 | compile/smoke | cmake --build build-vulkan --config Release --target mdcad_math_harness | ✅ | ⬜ pending |
| 06-01-02 | 01 | 1 | TAIL-01 | integration | {to be filled from generated PLAN tasks} | ❌ W0 | ⬜ pending |
| 06-01-03 | 01 | 1 | TAIL-01 | integration | {to be filled from generated PLAN tasks} | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] .planning/phases/06-serializer-and-save-load-long-tail-migration/evidence/serializer-roundtrip-checklist.md — repeatable targeted checks for entity count, parent links, transform fields, geometry types
- [ ] scripts/scene_format_convert.py (or equivalent) — old-format to cleaned-format converter workflow
- [ ] docs/QUICKSTART.md update — converter invocation and serializer targeted check steps

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Save+reload representative scene preserves hierarchy and transforms | TAIL-01 | Current repo has no dedicated serializer roundtrip test harness yet | Save scene from app, reload, verify entity count + parent links + transform fields + geometry types using checklist |
| Converted old-format scene loads under cleaned schema | TAIL-01 | Converter workflow is phase-specific and new | Run converter on old sample, load converted output, verify same targeted checklist fields |

---

## Validation Sign-Off

- [ ] All tasks have <automated> verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 300s
- [ ] 
yquist_compliant: true set in frontmatter

**Approval:** pending
