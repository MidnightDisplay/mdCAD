---
phase: 15
slug: validation-and-acceptance-closure
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-02
---

# Phase 15 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + manual reproducibility walkthroughs |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `cmake --build build-vulkan --config Release --target mdCAD` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | build ~60-180s; full suite ~60-240s |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build-vulkan --config Release --target mdCAD`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 240 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 15-01-01 | 01 | 1 | VAL-01 | manual/doc + artifact audit | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |
| 15-01-02 | 01 | 1 | VAL-02 | build gate | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ W0 | ⬜ pending |
| 15-02-01 | 02 | 2 | VAL-02 | full suite gate | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 15-03-01 | 03 | 3 | VAL-03 | deferred-traceability | `N/A (deferred by Phase 15 decision)` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `.planning/phases/15-validation-and-acceptance-closure/15-03-SUMMARY.md` — phase closure narrative with requirement-level outcomes.
- [ ] `.planning/phases/15-validation-and-acceptance-closure/evidence/windows-vulkan-msvc/gate-build-mdcad.txt` — Release `mdCAD` build output evidence.
- [ ] `.planning/phases/15-validation-and-acceptance-closure/evidence/windows-vulkan-msvc/gate-ctest-full.txt` — full CTest output evidence.
- [ ] `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/` — two sketch case studies + one Script IO scenario with reproducible steps.
- [ ] `CHECKPOINT.md` — update continuity entry with Phase 15 outcomes.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Two representative sketch/script case studies are reproducible for dev/debug | VAL-01 | Requires interactive workflow confirmation and artifact usability review | Open each case-study runbook, execute steps in app, and confirm expected outcomes/diagnostics match artifact notes. |
| Script IO case study demonstrates parse/apply/reset behavior and diagnostics visibility | VAL-01 | UI/runtime behavior needs human confirmation | Execute Script IO scenario steps, including one invalid edit, then verify diagnostics and reset/discard behavior in UI. |
| macOS parity status is explicitly deferred (not silently omitted) | VAL-03 | This is a process/audit correctness check | Confirm `15-VALIDATION.md`, `15-03-SUMMARY.md`, and `CHECKPOINT.md` all state deferred status with follow-up note. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 240s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
