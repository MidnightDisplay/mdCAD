---
phase: 07
slug: import-pipeline-long-tail-migration
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-03-27
---

# Phase 07 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Native CMake targets + targeted importer workflow evidence |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdcad_math_harness` |
| **Full suite command** | `cmake --build build-vulkan --config Release --target math-validation` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdcad_math_harness`
- **After every plan wave:** Run `cmake --build build-vulkan --config Release --target math-validation`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | TAIL-02 | compile + source-audit | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ✅ | ✅ green |
| 07-02-01 | 02 | 2 | TAIL-02 | targeted importer checks | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ✅ | ✅ green |
| 07-03-01 | 03 | 3 | TAIL-02 | native workflow evidence (task-level quick gate) | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `.planning/phases/07-import-pipeline-long-tail-migration/evidence/importer-targeted-checklist.md` — checklist for JSONL/PLY placement/orientation/scale/count/parenting verification.
- [x] `.planning/phases/07-import-pipeline-long-tail-migration/evidence/importer-targeted-check-report.md` — executed report with pass/fail outcomes and delta notes.
- [x] `docs/QUICKSTART.md` update — Phase 7 importer migration workflow commands and evidence pointers.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| JSONL import placement/orientation correctness | TAIL-02 | Requires viewport observation of spatial output | Import representative JSONL sample, compare placement/orientation against baseline notes, record pass/fail in evidence report |
| PLY point-cloud and mesh parenting/layout parity | TAIL-02 | Hierarchy and visual topology are runtime-visible workflows | Import representative and variant PLY files in both supported modes, verify entity/triangle counts and parenting structure in scene hierarchy, record results |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** ready
