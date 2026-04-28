# Phase 40 Stress Protocol (Very-large JSONL Flat Import)

## Purpose
Capture one manual very-large stress pass for flat JSONL observer refreshes with the Phase 40 hybrid gate:
- **Hard gate:** no lockup, no crash
- **Advisory gate:** refresh timing is evidence only and does not fail the run by itself
- **Hard-gate shorthand:** no lockup/crash

## Preconditions
- Build: `build-vulkan` (Release)
- App launches and flat JSONL import path is available
- Import dialog uses linked source path with observing enabled

## Dataset
Use one Very-large JSONL flat dump (target: hundreds of thousands of geometries).

Record:
- File path:
- Approx line/entry count:
- File size:

## Manual Procedure
1. Launch app and import the large file through the flat JSONL observable flow.
2. Confirm entities are imported under one flat anchor root.
3. Confirm source link is present and observing is enabled.
4. Trigger multiple source updates (or repeated manual refreshes) while keeping hierarchy/selection visible.
5. During active refreshes, attempt anchor-structure actions from hierarchy (drag/drop, delete, parent edits) and confirm lock behavior.
6. Select a child entity before refresh, refresh again, and confirm selection coherence to anchor root after replacement.
7. Let refresh cycles settle; confirm inspector shows refresh status and advisory timing copy.

## Evidence Capture
- Hard gate (no lockup/no crash): PASS / FAIL
- UI remained responsive during refresh: PASS / FAIL
- Anchor coherence after repeated cycles: PASS / FAIL
- Selection remap child -> anchor root after replacement: PASS / FAIL
- Hierarchy guardrails during running refresh: PASS / FAIL
- Advisory timings observed (examples):
  - Refresh sample 1:
  - Refresh sample 2:
  - Refresh sample 3:
- Notes:

## Hybrid Gate Interpretation
- Mark run **FAILED** only for hard-gate violations (lockup/crash) or coherence breaks.
- Treat high advisory timings as optimization signals; record them, but do not fail solely on timing.

## Manual Run Evidence (Checkpoint)
- Evidence source: user checkpoint approval in execute-phase flow
- Hard gate (no lockup/no crash): PASS
- UI remained responsive during refresh: PASS
- Anchor coherence after repeated cycles: PASS
- Selection remap child -> anchor root after replacement: PASS
- Hierarchy guardrails during running refresh: PASS
- Advisory timings observed: PASS (inspector advisory timing channel present during run; treated as non-blocking evidence)
- Follow-up issues: none reported

## Manual run result:
APPROVED (user-confirmed pass).
