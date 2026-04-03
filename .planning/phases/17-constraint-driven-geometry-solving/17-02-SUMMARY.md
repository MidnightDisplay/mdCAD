---
phase: 17-constraint-driven-geometry-solving
plan: 02
subsystem: ui
tags: [endpoint-pick, constraint-authoring, checkpoint-failure, ux-gap]
requires:
  - phase: 17-constraint-driven-geometry-solving
    provides: Transactional solve contracts and baseline deterministic solver tests
provides:
  - Endpoint/sub-entity pick ID plumbing in normal viewport flow
  - Endpoint pick precedence regression coverage in endpoint_pick tests
  - Audited human-verification failure record with architecture redesign proposal
affects: [17-VALIDATION, endpoint UX, phase-17 gap planning]
tech-stack:
  added: []
  patterns: [endpoint pick IDs decoded into constraint participants, overlay-point priority for endpoint picks]
key-files:
  created:
    - .planning/phases/17-constraint-driven-geometry-solving/17-02-SUMMARY.md
  modified:
    - .planning/phases/17-constraint-driven-geometry-solving/17-VALIDATION.md
    - .planning/phases/17-constraint-driven-geometry-solving/17-DISCUSSION-LOG.md
key-decisions:
  - "Checkpoint Task 3 is NOT approved; Plan 17-02 remains incomplete pending redesign."
  - "Route follow-up through gap planning to replace special endpoint pick tail-range approach with native point entities under sketch-owned EndPoints component."
patterns-established:
  - "Automated ctest pass is insufficient for endpoint UX closure; manual viewport visibility/selection behavior is a hard gate."
requirements-completed: []
duration: 14 min
completed: 2026-04-03
---

# Phase 17 Plan 02: Endpoint authoring flow Summary (Checkpoint Failed)

**Endpoint pick contracts and regression tests landed, but human verification failed due to invisible endpoint rendering and invalid point-context menu semantics, requiring architectural redesign before closure.**

## Performance

- **Duration:** 14 min
- **Started:** 2026-04-03T00:00:00Z
- **Completed:** 2026-04-03T00:14:00Z
- **Tasks:** 2/3 complete (Task 3 failed checkpoint)
- **Files modified:** 3 (docs/validation artifacts)

## Accomplishments
- Task 1 endpoint/sub-entity pick contracts and constraint participant integration were committed.
- Task 2 deterministic endpoint overlay precedence tests and implementation were committed.
- Checkpoint failure details and redesign direction were captured for planning consumption.

## Task Commits

1. **Task 1 (TDD RED): Add endpoint/sub-entity pick contract tests** - `1cc6028` (test)  
2. **Task 1 (TDD GREEN): Wire endpoint sub-entity picks into constraint authoring** - `3c3e533` (feat)  
3. **Task 2 (TDD RED): Add endpoint precedence regression tests** - `d20a2ee` (test)  
4. **Task 2 (TDD GREEN): Codify deterministic endpoint overlay precedence** - `3b8378e` (feat)  

## Checkpoint Result (Task 3: human-verify)

**Status:** ❌ Failed / Not approved  
**Automated verify:** ✅ `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure`

### User-reported failures
1. Endpoint points are not visibly rendered in normal viewport; they are only observable via pick IDs or gizmo Tab mode.
2. Left-click near endpoint opens constraint context menu as if point-selected + C-triggered.
3. Single endpoint selection exposes invalid menu options for a point (line-oriented options such as Fixed/Along X/Y/Z/Length).

### Proposed redesign (user-provided, preserved verbatim in intent)
1. Add a new `EndPoints` component attached only to sketch geometry entities.
2. Spawn notable vertices as real point entities parented to their geometry entity.
3. Keep those point entities transform-synced to line/arc notable vertices.
4. Use native point entities for hover/select/multiselect/gizmo without Tab vertex mode.
5. Keep normal pick buffer allocation; remove special endpoint tail-range handling.
6. Let these points participate naturally in coincident/colinear/along-axis/length constraints.
7. Scope to sketch-created lines/arcs only; do not alter bare non-sketch line/arc behavior.

## Decisions Made
- Task 3 checkpoint is treated as a blocking failure, not an approval.
- Plan 17-02 is left incomplete and should be re-planned via a gap/revision flow.

## Deviations from Plan

### Checkpoint-driven scope correction

**1. [Rule 4 - Architectural Change] Existing endpoint sub-entity strategy is inadequate for UX correctness**
- **Found during:** Task 3 (human verification)
- **Issue:** Current endpoint representation does not provide visible, first-class point entity behavior in normal viewport flow.
- **Action:** Execution stopped; failure and redesign proposal recorded for planning.
- **Files modified:** `17-02-SUMMARY.md`, `17-VALIDATION.md`, `17-DISCUSSION-LOG.md`
- **Committed in:** _(this docs commit)_

---

**Total deviations:** 1 (architectural decision required)  
**Impact on plan:** Plan 17-02 cannot be marked complete; redesign planning is required before implementation continuation.

## Issues Encountered
- Human-verification UX behavior contradicted D-09..D-12 intent despite passing automated endpoint_pick tests.

## Auth Gates
None.

## Next Phase Readiness
- Not ready to close Phase 17.
- Recommended next command: **`/gsd-plan-phase 17 --gaps`** to generate a redesign plan grounded in this checkpoint failure.

## Self-Check: PASSED
- Confirmed existing task commits are present in git history (`1cc6028`, `3c3e533`, `d20a2ee`, `3b8378e`).
- Confirmed summary file exists at `.planning/phases/17-constraint-driven-geometry-solving/17-02-SUMMARY.md`.
