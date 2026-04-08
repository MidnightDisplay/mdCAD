# Feature Research: v1.4 Solver Robustness + Sketch Gizmo Corrections

**Domain:** Sketch constraint correctness, solver reliability, and sketch-line gizmo semantics  
**Researched:** 2026-04-08  
**Confidence:** High

## Category 1 - Line-Line Parallel / Perpendicular

### Table Stakes

- Pair selection can create valid `PARALLEL` / `PERPENDICULAR` constraints.
- Group selections resolve deterministically and remain stable under drag.
- Invalid selections are rejected with actionable diagnostics.

### Anti-Features

- Selection-order dependent outcomes.
- Hidden over-constraint expansion without clear diagnostics.

## Category 2 - Line ALONG X/Y/Z

### Table Stakes

- Applying ALONG to lines must not trigger immediate unsatisfied-driving failures.
- Principal-axis intent must be preserved in mixed-constraint sketches.
- Equivalent drags from mirrored vertices should behave consistently.

### Anti-Features

- Immediate `[ERROR] solve Unsatisfied driving ALONG *` on valid authoring.
- Side-dependent behavior where one valid drag works and equivalent drag fails.

## Category 3 - Arc-Line Tangency Robustness

### Table Stakes

- Tangency authoring in typical fillet-like setups remains solvable.
- Dragging endpoints keeps tangency stable when feasible.
- Infeasible edits fail transactionally without global solver stall.

### Anti-Features

- Solver stalls after one failed tangency interaction.
- Branch flip/jitter behavior during drag.

## Category 4 - Active-Sketch Line Gizmo

### Table Stakes

- Gizmo anchored at line midpoint for selected line in active sketch.
- Drag updates geometry endpoints `A/B` in sync (not transform-only drift).
- Undo remains gesture-coherent.

### Anti-Features

- Transform updates without matching geometry updates.
- Midpoint mismatch between visual line and gizmo origin.

## Category 5 - Solver Architecture Documentation

### Table Stakes

- Easy-read architecture overview for human consumers.
- Literature references plus direct code-structure mapping.
- TL;DR primer for key constraint implementation patterns.

### Anti-Features

- Theory-only docs disconnected from code.
- Missing failure taxonomy for practical debugging.
