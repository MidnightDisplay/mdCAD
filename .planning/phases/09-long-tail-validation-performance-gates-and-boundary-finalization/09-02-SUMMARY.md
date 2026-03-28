---
phase: 09-long-tail-validation-performance-gates-and-boundary-finalization
plan: 02
subsystem: testing
tags: [performance, benchmark, validation, vulkan, metal, harness]
requires:
  - phase: 09-01
    provides: long-tail strict compare coverage map and evidence baseline
provides:
  - Fresh Phase 9 benchmark candidate artifacts for required native targets
  - Deterministic per-case VAL-02 gate evaluations with explicit OVERALL decisions
  - Blocking evidence for unresolved Windows regression and unavailable macOS host capture
affects: [09-03, VAL-02, phase-closure]
tech-stack:
  added: []
  patterns: [per-case no-regression gating, provenance-first perf evidence capture]
key-files:
  created:
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/bench-run1.txt
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/provenance.txt
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-run1.txt
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/provenance.txt
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-eval.md
  modified:
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/baseline/bench-run1.txt
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/baseline/provenance.txt
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/baseline/bench-run1.txt
    - .planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/baseline/provenance.txt
key-decisions:
  - "Publish explicit OVERALL FAIL decisions instead of masking unresolved regression/blockage."
  - "Treat unavailable macOS host as blocking evidence status with complete artifact structure and provenance."
patterns-established:
  - "VAL-02 evidence format requires baseline+candidate+provenance+bench-eval per target even when blocked."
requirements-completed: []
duration: 7 min
completed: 2026-03-28
---

# Phase 9 Plan 02: Execute native benchmark/perf gates for expanded migration slice Summary

**Per-target per-case benchmark evidence was captured and evaluated for Phase 9, producing explicit blocking FAIL outcomes for Windows regression and macOS host unavailability.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-03-28T13:06:57Z
- **Completed:** 2026-03-28T13:13:58Z
- **Tasks:** 2
- **Files modified:** 10

## Accomplishments
- Captured fresh Windows Vulkan candidate benchmark output at `--iterations 2000000` with full provenance metadata.
- Created complete Phase 9 perf evidence trees for both required targets (baseline/candidate/provenance/evaluation layout).
- Published deterministic per-case gate evaluations with explicit `OVERALL: FAIL` status for both targets, with blocking rationale documented.

## Task Commits

Each task was committed atomically:

1. **Task 1: Capture fresh high-iteration candidate benchmark artifacts for both required targets** - `a8914bd` (perf)
2. **Task 2: Evaluate per-case no-regression gates and publish VAL-02 decisions** - `74285f0` (docs)

**Plan metadata:** Recorded in final `docs(09-02)` metadata commit.

## Files Created/Modified
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/baseline/bench-run1.txt` - Seeded Phase 9 baseline input for evaluator.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/baseline/provenance.txt` - Baseline provenance copied into Phase 9 target tree.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/bench-run1.txt` - Fresh high-iteration Windows candidate capture.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/provenance.txt` - Windows candidate provenance with commit/cmd/host metadata.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md` - Deterministic per-case evaluation with `OVERALL: FAIL`.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/baseline/bench-run1.txt` - Seeded Phase 9 baseline input for evaluator.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/baseline/provenance.txt` - Baseline provenance copied into Phase 9 target tree.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-run1.txt` - Blocking placeholder capture documenting unavailable macOS host.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/provenance.txt` - macOS blocked provenance with explicit blocking reason.
- `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-eval.md` - Explicit blocked evaluation artifact with `OVERALL: FAIL`.

## Decisions Made
- Kept evaluator policy deterministic/per-case and did not replace case-level decisions with aggregate summary claims.
- Marked unresolved regressions and missing required host evidence as explicit blocking FAIL outcomes per D-03/D-05 policy.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Required macOS Metal host unavailable for native candidate capture**
- **Found during:** Task 1
- **Issue:** Current execution environment is Windows-only, so native macOS `mdcad_math_harness` candidate run could not be produced locally.
- **Fix:** Created complete macOS candidate artifact structure with blocked status in `bench-run1.txt` and `provenance.txt`, then published explicit blocking `bench-eval.md` with `OVERALL: FAIL` (no fake pass).
- **Files modified:** `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-run1.txt`, `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/provenance.txt`, `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-eval.md`
- **Verification:** Provenance includes `capture_status=blocked`; bench-eval includes `OVERALL: FAIL`.
- **Committed in:** `a8914bd` (task artifact setup) and `74285f0` (evaluation publish)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Preserved truthful VAL-02 evidence and gating semantics; requirement remains blocked pending macOS-native capture and Windows regression resolution.

## Issues Encountered
- Windows per-case gate result flagged `bench-interaction-drag` at 6.034% slowdown vs 5% threshold, yielding blocking `OVERALL: FAIL`.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 9 perf evidence is complete in structure and deterministic evaluation output.
- Remaining blocker for milestone closure: resolve Windows `bench-interaction-drag` regression and run true macOS native candidate capture.

## Self-Check: PASSED

- Verified required perf evidence files exist for both targets (baseline/candidate/provenance/eval).
- Verified both `bench-eval.md` artifacts contain explicit `OVERALL` gate decisions.
- Verified task commits exist in git history: `a8914bd`, `74285f0`.

---
*Phase: 09-long-tail-validation-performance-gates-and-boundary-finalization*
*Completed: 2026-03-28*
