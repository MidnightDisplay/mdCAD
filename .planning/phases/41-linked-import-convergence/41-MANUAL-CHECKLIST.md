# Phase 41 Manual Checklist - lamp_11 linked-import convergence

## Purpose

Use this checklist to prove the Phase 41 linked flat-import fix against the real large dataset before closing the phase.

## Required source file

`C:\Users\RodionRadchenko\source\repos\ParkerSteel-GenerateDimensionedDrawing\ParkerSteel.Drawing.Test\bin\Debug\net8.0-windows\lamp_11.jsonl`

## Rules for this pass

1. Launch the current `build-vulkan` Release app build.
2. Import through `File -> Import JSONL (Flat Large Dump)...`.
3. Enable `Link file for refresh (optional)` before starting the import.
4. **Do not edit, rewrite, rename, or otherwise touch the source file after the import starts.** This pass validates the initial linked-import convergence only, not later refresh-after-change behavior.

## Manual procedure

1. Launch the app and open `File -> Import JSONL (Flat Large Dump)...`.
2. Choose the exact `lamp_11.jsonl` path above.
3. Turn on `Link file for refresh (optional)`.
4. Start the import and wait until the imported geometry first appears committed in the viewport.
5. Record the first committed `Scene Hierarchy` total for the linked import root (`Total: N lines, M points`).
6. Keep the source file untouched and continue watching until the import is clearly settled. Record the later settled `Scene Hierarchy` total for the same root.
7. Confirm the viewport stays fully populated after settling. Fail this step if the result collapses to a tail subset, points-only remnants, or any other visibly reduced import.
8. Open `Slot Buffer Debug` and inspect the line/point slot occupancy after the import settles. Fail this step if a large early-slot chunk disappears or only the tail of the imported data remains.
9. Select the linked import root and review `JSONL Flat Refresh` in `Entity Inspector`. Record:
   - `Source path:`
   - `Observe automatically`
   - `Last refresh result`
10. Fill the evidence fields below and replace every `TBD` marker.

## Evidence fields

- Viewport result: TBD
  - PASS = full committed geometry remains visible after the import settles.
  - FAIL = visible result collapses, drops major geometry, or leaves points-only remnants.

- Scene Hierarchy first committed total: TBD

- Scene Hierarchy settled total: TBD

- Slot Buffer note: TBD
  - PASS = slot occupancy stays consistent after settling with no disappearing early-slot chunk.
  - FAIL = slot occupancy drops a large early chunk, tail-only remainder, or dangling remnants.

- Observer status note: TBD
  - Record the exact `Source path`, whether `Observe automatically` is on or off, and the latest `Last refresh result` message.

- Final verdict: TBD
  - PASS only if the viewport remains complete, the first committed and settled hierarchy totals converge instead of continuing to climb, the slot buffers stay stable, and the observer status is consistent with a settled linked import.
  - FAIL for any geometry collapse, continued count churn, slot-buffer instability, or unexpected observer state.
