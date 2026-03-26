---
phase: 06-serializer-and-save-load-long-tail-migration
plan: 02
subsystem: tooling
tags: [converter, serializer, validation-checklist, quickstart]
requires:
  - phase: 06-serializer-and-save-load-long-tail-migration
    provides: serializer schema v2 contract and strict format/version enforcement from 06-01
provides:
  - CLI converter utility to migrate legacy scenes into cleaned schema v2
  - Repeatable serializer targeted roundtrip checklist for representative and converted scenes
  - Quickstart runbook section for Phase 6 converter + targeted checks
affects: [06-03-validation-evidence, TAIL-01]
tech-stack:
  added: []
  patterns:
    - converter fails fast on invalid input/schema instead of silently guessing
    - serializer light-gate validation recorded through explicit checklist fields
key-files:
  created:
    - scripts/scene_format_convert.py
    - .planning/phases/06-serializer-and-save-load-long-tail-migration/evidence/serializer-roundtrip-checklist.md
    - .planning/phases/06-serializer-and-save-load-long-tail-migration/06-02-SUMMARY.md
  modified:
    - docs/QUICKSTART.md
key-decisions:
  - "Implement converter as a Python CLI in scripts/ for fast operator workflow and deterministic migration."
  - "Constrain checklist to D-08 targeted fields and preserve light-gate policy for Phase 6."
  - "Document Phase 6 runbook in QUICKSTART without broadening into importer/undo/editor guidance."
patterns-established:
  - "Phase migration tooling pattern: converter utility + explicit checklist + quickstart pointer."
requirements-completed: [TAIL-01]
duration: 1 min
completed: 2026-03-26
---

# Phase 06 Plan 02 Summary

**Phase 6 now includes a deterministic scene schema converter, a concrete serializer roundtrip checklist, and Quickstart instructions that make the cleaned-schema workflow executable end-to-end.**

## Performance

- **Duration:** 1 min
- **Started:** 2026-03-26T23:43:18Z
- **Completed:** 2026-03-26T23:44:20Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Added `scripts/scene_format_convert.py` with `--input`/`--output` flags, schema validation, and fail-fast error handling.
- Added `serializer-roundtrip-checklist.md` covering entity count, parent links, transforms, geometry type, and converted-scene branch.
- Updated `docs/QUICKSTART.md` with a dedicated Phase 6 converter + targeted check workflow.

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement old-scene to cleaned-scene converter utility** - `0233ea9` (feat)
2. **Task 2: Create targeted serializer roundtrip checklist artifact** - `419cb3e` (docs)
3. **Task 3: Update Quickstart with converter and targeted check workflow** - `3c1ae91` (docs)

## Files Created/Modified
- `scripts/scene_format_convert.py` - converts legacy scene JSON to schema v2 and validates structure/fields.
- `.planning/phases/06-serializer-and-save-load-long-tail-migration/evidence/serializer-roundtrip-checklist.md` - targeted Phase 6 validation checklist.
- `docs/QUICKSTART.md` - Phase 6 serializer runbook section with converter command and checklist path.

## Decisions Made
- Keep converter behavior strict and explicit to avoid accidental acceptance of unsupported scene formats.
- Keep Quickstart additions scoped to Phase 6 serializer workflow only.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Plan 06-03 can execute compile + targeted evidence capture using the new checklist and converter.
- Required artifacts for D-08 validation are now in place and referenced from Quickstart.

---
*Phase: 06-serializer-and-save-load-long-tail-migration*
*Completed: 2026-03-26*
