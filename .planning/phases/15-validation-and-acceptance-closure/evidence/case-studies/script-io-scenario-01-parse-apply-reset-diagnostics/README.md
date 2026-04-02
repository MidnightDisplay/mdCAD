# Script IO Scenario 01 — parse/apply/reset diagnostics

## Purpose

Exercise Script IO parse/apply/reset behavior with explicit diagnostic visibility in one reproducible workflow.

## Repro Steps

1. Open a sketch in `mdCAD`.
2. Open Script Editor and paste `script.lua` from this folder.
3. Click **Apply Script** and confirm success.
4. Open **Script IO** window.
5. Change `input_length` to `12.5` and confirm geometry/output update.
6. Introduce a parse error in Script Editor (for example remove a closing `}` from the `constraints` block).
7. Click **Apply Script** and confirm diagnostic appears.
8. Use reset/discard workflow to return to last valid script state.

## Expected Outcome

- Initial apply succeeds and IO window lists numeric input/output values.
- IO edit auto-applies through transactional pipeline.
- Deliberate syntax damage produces parse diagnostics.
- Reset/discard restores last committed valid script and scene state.

## Diagnostics

- Parse failure should report explicit message such as missing matching `}`.
- Failed apply must not corrupt prior valid scene/script state.
- Subsequent valid apply after reset should resume normal behavior.

