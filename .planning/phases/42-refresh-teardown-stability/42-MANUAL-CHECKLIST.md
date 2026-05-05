# Phase 42 Manual Checklist - lamp_11 refresh/delete lifecycle

## Purpose

Use this checklist to close Phase 42 with explicit real-file evidence for linked auto-reload, repeated manual refresh, delete-after-refresh cleanup, and delete-during-refresh cancellation.

## Required source file

`C:\Users\RodionRadchenko\source\repos\ParkerSteel-GenerateDimensionedDrawing\ParkerSteel.Drawing.Test\bin\Debug\net8.0-windows\lamp_11.jsonl`

## Required build and automated gate

1. Run:
   `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(auto_safety_test|manual_refresh_test|inspector_contract_test)" --output-on-failure`
2. Launch the current `build-vulkan` Release app build.

## Rules for this pass

1. Import through `File -> Import JSONL (Flat Large Dump)...`.
2. Enable `Link file for refresh (optional)` before every linked import used in this pass.
3. Keep `Scene Hierarchy`, `Entity Inspector`, and `Slot Buffer Debug` available while collecting evidence.
4. For Scenario 1, rewrite or regenerate the exact same watched `lamp_11.jsonl` path once while `Observe automatically` stays ON so auto-reload actually fires.
5. Wait for every refresh to settle before recording evidence unless the scenario explicitly says to delete while refresh is active.
6. Replace every `TBD` marker before closing the phase.

## Scenario 1 - linked auto-reload

### Procedure

1. Launch the app and import the exact `lamp_11.jsonl` path above through `File -> Import JSONL (Flat Large Dump)...`.
2. Turn on `Link file for refresh (optional)` before starting the import.
3. Wait for the initial linked import to settle.
4. Select the linked root and confirm `Observe automatically` is ON in `Entity Inspector`.
5. Externally rewrite or regenerate the same watched `lamp_11.jsonl` path once so auto-reload triggers.
6. Wait until the refresh settles again.
7. Record viewport coverage, the first settled vs final settled `Scene Hierarchy` totals, the `Slot Buffer Debug` line/point stats (`Total slots / Capacity / Free`), and the latest `Last refresh result` message.

### Evidence

- Viewport result: TBD
- Scene Hierarchy evidence: TBD
- Slot Buffer evidence: TBD
- Observer/Inspector evidence: TBD
- Scenario verdict: TBD

## Scenario 2 - repeated manual Re-import now

### Procedure

1. Keep the same linked root selected in `Entity Inspector`.
2. Press `Re-import now` at least three times.
3. Wait for each run to settle fully before starting the next one.
4. After each settle, record whether viewport coverage, `Scene Hierarchy` totals, and `Slot Buffer Debug` stats return to the same final footprint.
5. Record the latest `Last refresh result` message after the third settle.

### Evidence

- Viewport result: TBD
- Scene Hierarchy evidence: TBD
- Slot Buffer evidence: TBD
- Observer/Inspector evidence: TBD
- Scenario verdict: TBD

## Scenario 3 - delete after settled refresh history

### Procedure

1. Start from the linked root after Scenario 2 has settled.
2. Delete the linked root from `Scene Hierarchy` or with the Delete key.
3. Confirm the viewport geometry disappears in the same action.
4. Confirm the linked root entry disappears from `Scene Hierarchy`.
5. Open `Slot Buffer Debug` and record whether the deleted root's slot ownership is gone immediately.
6. Record any relevant inspector or status-message evidence before or after deletion.

### Evidence

- Viewport result: TBD
- Scene Hierarchy evidence: TBD
- Slot Buffer evidence: TBD
- Observer/Inspector evidence: TBD
- Scenario verdict: TBD

## Scenario 4 - delete while refresh is active

### Procedure

1. If Scenario 3 deleted the only linked root, re-import the same `lamp_11.jsonl` path with linking enabled and wait for the initial settle.
2. Select the linked root in `Entity Inspector`.
3. Start a manual refresh with `Re-import now`.
4. While `Entity Inspector` shows `Refresh in progress...` or the `Scene Hierarchy` row shows `(refreshing)`, delete the linked root.
5. Confirm the delete cancels the running refresh and leaves no visible leftovers in the viewport or slot-buffer state.
6. Press undo once.
7. Confirm undo restores only the last committed import state, not any partial staged refresh result.
8. Record the latest `Last refresh result` message, including any cancellation message.

### Evidence

- Viewport result: TBD
- Scene Hierarchy evidence: TBD
- Slot Buffer evidence: TBD
- Observer/Inspector evidence: TBD
- Scenario verdict: TBD

## Overall verdict

Overall verdict: TBD
