---
phase: 15
slug: validation-and-acceptance-closure
status: complete
nyquist_compliant: true
wave_0_complete: true
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
| 15-01-01 | 01 | 1 | VAL-01 | manual/doc + artifact audit | `cmake --build build-vulkan --config Release --target mdCAD` | ✅ | ✅ green |
| 15-01-02 | 01 | 1 | VAL-01 | validation mapping | `git --no-pager grep -n "sketch-01-constraint-debug\\|sketch-02-driven-dimensions\\|script-io-scenario-01-parse-apply-reset-diagnostics" .planning/phases/15-validation-and-acceptance-closure/15-VALIDATION.md` | ✅ | ✅ green |
| 15-02-01 | 02 | 2 | VAL-02 | build gate evidence | `cmake --build build-vulkan --config Release --target mdCAD` | ✅ | ✅ green |
| 15-02-02 | 02 | 2 | VAL-02 | full suite gate evidence | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 15-03-01 | 03 | 3 | VAL-03 | deferred-traceability | `N/A (deferred by Phase 15 decision)` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `.planning/phases/15-validation-and-acceptance-closure/15-03-SUMMARY.md` — phase closure narrative with requirement-level outcomes.
- [x] `.planning/phases/15-validation-and-acceptance-closure/evidence/windows-vulkan-msvc/gate-build-mdcad.txt` — Release `mdCAD` build output evidence.
- [x] `.planning/phases/15-validation-and-acceptance-closure/evidence/windows-vulkan-msvc/gate-ctest-full.txt` — full CTest output evidence.
- [x] `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/` — two sketch case studies + one Script IO scenario with reproducible steps.
- [x] `CHECKPOINT.md` — update continuity entry with Phase 15 outcomes.

---

## Requirement Evidence Mapping

### VAL-01 — Example sketch/script case studies shipped

Required composition satisfied per D-06 and D-07:
- Two representative sketch case studies:
  - `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/sketch-01-constraint-debug/README.md`
  - `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/sketch-02-driven-dimensions/README.md`
- One Script IO parse/apply/reset/diagnostics scenario:
  - `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/script-io-scenario-01-parse-apply-reset-diagnostics/README.md`

Script artifacts referenced by runbooks:
- `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/sketch-01-constraint-debug/sketch.lua`
- `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/sketch-02-driven-dimensions/sketch.lua`
- `.planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/script-io-scenario-01-parse-apply-reset-diagnostics/script.lua`

Verification commands:
- `Get-ChildItem .planning/phases/15-validation-and-acceptance-closure/evidence/case-studies -Recurse`
- `git --no-pager grep -n "parse/apply/reset" .planning/phases/15-validation-and-acceptance-closure/evidence/case-studies/script-io-scenario-01-parse-apply-reset-diagnostics/README.md`

Status: ✅ complete (artifact package created and mapped)

### VAL-02 — Windows MSVC + Vulkan acceptance gates

Required commands per D-03/D-04 and recorded outcomes:

1. Build gate command:
   - `cmake --build build-vulkan --config Release --target mdCAD`
   - Evidence: `.planning/phases/15-validation-and-acceptance-closure/evidence/windows-vulkan-msvc/gate-build-mdcad.txt`
   - Anchor: `mdCAD.vcxproj -> C:\dev\mdCAD\build-vulkan\bin\Release\mdCAD.exe`
   - Exit code: `0`

2. Full suite command:
   - `ctest --test-dir build-vulkan -C Release --output-on-failure`
   - Evidence: `.planning/phases/15-validation-and-acceptance-closure/evidence/windows-vulkan-msvc/gate-ctest-full.txt`
   - Anchor: `100% tests passed, 0 tests failed out of 1`
   - Exit code: `0`

Provenance:
- `.planning/phases/15-validation-and-acceptance-closure/evidence/windows-vulkan-msvc/provenance.md`

Status: ✅ complete (both mandatory commands passed and were captured)

---

### VAL-03 — macOS parity validation status (deferred in Phase 15)

Deferred by locked discuss-phase decisions D-09 and D-10:
- **Status in Phase 15:** ⏸ deferred / out of scope (no completion claim).
- **Reason:** User-directed scope for this phase prioritizes Windows evidence closure first.
- **Handoff:** Execute macOS parity validation in a follow-up closure activity after Phase 15.
- **Traceability anchors:** `15-03-SUMMARY.md` and `CHECKPOINT.md` include matching deferred language and follow-up pointer.

Status: ✅ complete for deferred-traceability documentation contract.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Two representative sketch/script case studies are reproducible for dev/debug | VAL-01 | Requires interactive workflow confirmation and artifact usability review | Open each case-study runbook, execute steps in app, and confirm expected outcomes/diagnostics match artifact notes. |
| Script IO case study demonstrates parse/apply/reset behavior and diagnostics visibility | VAL-01 | UI/runtime behavior needs human confirmation | Execute Script IO scenario steps, including one invalid edit, then verify diagnostics and reset/discard behavior in UI. |
| macOS parity status is explicitly deferred (not silently omitted) | VAL-03 | This is a process/audit correctness check | Confirm `15-VALIDATION.md`, `15-03-SUMMARY.md`, and `CHECKPOINT.md` all state deferred status with follow-up note. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 240s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete
