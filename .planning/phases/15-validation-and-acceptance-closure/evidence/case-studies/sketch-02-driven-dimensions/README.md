# Sketch Case Study 02 — Driven Dimensions (Length + Angle)

## Purpose

Provide a dimension-oriented sketch case study for validating length/angle constraints and driven-value behavior in script workflows.

## Repro Steps

1. Open `mdCAD` and select a sketch.
2. Open Script Editor and paste this folder's `sketch.lua`.
3. Click **Apply Script**.
4. Verify both constraints appear in ConstraintManager.
5. Toggle/value-edit dimension rows in UI as supported and observe updates.

## Expected Outcome

- Script applies successfully.
- Sketch contains one line and one arc/circle reference geometry.
- `Length` and `Angle` constraints are visible with configured values.
- Editing related geometry preserves stable script IDs on re-emit.

## Diagnostics Notes

- Illegal participant swaps (for example replacing line participant with unrelated unsupported geometry for the same constraint type) should be rejected with diagnostics.
- Failed apply attempts must preserve the previous committed state.

