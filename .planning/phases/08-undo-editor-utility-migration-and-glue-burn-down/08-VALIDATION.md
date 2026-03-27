---
phase: 08
slug: undo-editor-utility-migration-and-glue-burn-down
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-03-27
---

# Phase 08 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Native CMake targets + focused undo/editor workflow evidence |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdcad_math_harness` |
| **Full suite command** | `cmake --build build-vulkan --config Release --target math-validation` |
| **Estimated runtime** | ~90-240 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdcad_math_harness`
- **After every plan wave:** Run `cmake --build build-vulkan --config Release --target math-validation`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 300 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 08-01-01 | 01 | 1 | TAIL-03 | compile + migration-surface audit | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ✅ | ✅ green |
| 08-02-01 | 02 | 2 | TRED-01 | glue inventory + non-consumer proof + compile | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ✅ | ✅ green |
| 08-03-01 | 03 | 3 | TAIL-03, TRED-01 | targeted workflow validation + full suite | `cmake --build build-vulkan --config Release --target math-validation` | ✅ | ⚠️ partial (build gates green; manual workflow execution pending) |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/evidence/undo-editor-targeted-checklist.md` — focused manual checks for mandatory workflows.
- [x] `.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/evidence/undo-editor-targeted-check-report.md` — pass/fail report with correctness delta notes.
- [x] `.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/evidence/glue-inventory.md` — removed/retained/deferred glue ledger with rationale and carry-over notes.
- [x] `docs/QUICKSTART.md` update — Phase 8 execution and focused validation runbook.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Undo/redo transform edits preserve expected behavior after migration | TAIL-03 | Requires interactive editor operations (transform mutations + undo stack traversal) | Perform translate/rotate/scale edits, execute undo/redo cycles, confirm state restoration and stable scene behavior; log outcomes in targeted report |
| Gizmo vertex edit workflow parity after math migration | TAIL-03 | Vertex editing flow is interaction-heavy and requires viewport manipulation confirmation | Edit geometry vertices through gizmo mode, undo/redo changes, verify expected vertex positions and stable interaction behavior |
| Inspector edit workflow parity + command recording behavior | TAIL-03 | Inspector interactions include drag-start/end command capture semantics | Modify transform and geometry values in inspector, verify undo entries and state restoration behavior match expected intent |
| Glue burn-down safety and deferred carry-over accounting | TRED-01 | Evidence artifact review cannot be fully inferred from automated tests | Validate glue inventory reflects all removals/retentions/deferred items with explicit rationale and impacted consumers |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 300s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending manual workflow execution in `/gsd-verify-work 8`
