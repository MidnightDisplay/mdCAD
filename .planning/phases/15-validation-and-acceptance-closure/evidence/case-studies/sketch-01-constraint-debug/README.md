# Sketch Case Study 01 — Constraint Debug (Coincident + Perpendicular)

## Purpose

Provide a realistic constraint-focused sketch script that helps reproduce and debug participant-linking and legality issues.

## Repro Steps

1. Launch `mdCAD` on Windows Vulkan build.
2. Create or select a sketch entity.
3. Open Script Editor and paste `sketch.lua` from this folder.
4. Click **Apply Script**.
5. Inspect GeometryManager and ConstraintManager rows for expected entities and constraints.
6. Move one unconstrained point in viewport and re-run solver if needed.

## Expected Outcome

- Script applies without parse errors.
- Sketch contains points + lines with stable script IDs.
- Constraint list includes one `Coincident` and one `Perpendicular` relation.
- Constraint selection highlights all participant geometry.

## Diagnostics Notes

- If participant IDs are edited to unknown IDs, apply should fail with explicit diagnostics and no partial scene corruption.
- If a line participant is replaced by an unsupported type for `Perpendicular`, preview/apply should reject and preserve prior valid state.

