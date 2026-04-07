# Plan: Constraint Polish + Sketch Workspace UX (Post-Phase-11)

## Why

Phase 11 interaction/UAT is now stable, but user validation surfaced follow-up UX/data-model gaps:

1. Numeric fields accept non-numeric input in constraint value editors.
2. Some constraints should support multi-entity membership (e.g. parallel/collinear >2 entities).
3. Geometry deletion should preserve constraints while still valid (>=2 linked geometries where applicable).
4. Geometry/constraint entities need direct linked-relationship management in Entity Inspector.
5. Multiple glyphs on one geometry should stack horizontally to the right in stable order.
6. SketchManager/GeometryManager/ConstraintManager should support an active-sketch standalone workspace window with mutual-exclusion activation.
7. Scene Hierarchy should show `Lights` subtree above main scene tree.
8. App should boot with a predefined example sketch scene for quick validation/demo.

## Scope

In scope:
- Constraint input hardening, relationship semantics, inspector relationship panes, glyph layout policy, active sketch workspace UX, hierarchy order, startup sample scene.

Out of scope:
- Solver backend behavior changes, script subsystem behavior changes, major roadmap re-phasing.

## Implementation Strategy

### Wave 1 — Stability + Data Contracts

#### Task W1-1: Strict numeric entry for dimensional constraints
- Update dimensional inputs in both:
  - `ConstraintManager` row editor (`ui_entity_inspector.h`)
  - viewport popup editor (`app.c`)
- Enforce decimal-only text entry and reject alpha input at source.
- Keep existing `float` storage and mirrored update path.

Acceptance:
- Entering letters is blocked.
- Decimal, sign, and normal numeric edits still work.

#### Task W1-2: Safe geometry deletion with partial constraint retention
- Replace current "always delete linked constraints" behavior with arity-aware cleanup:
  - For n-ary-capable constraints (parallel/collinear), remove deleted participant(s), keep constraint if still valid (>=2).
  - For binary/single constraints, delete constraint when participant count falls below legal minimum.
- Centralize legality/min-participant policy in constraint helper layer.
- Ensure no dangling participant refs or stale glyph entries.

Acceptance:
- Deleting one geometry from a 3-line parallel set preserves constraint with remaining 2 lines.
- No crash in glyph-select + delete flow.
- Clear Scene remains crash-free.

#### Task W1-3: N-ary applicability support for selected constraint types
- Extend legality checks for:
  - `PARALLEL` (2+ lines)
  - `COLLINEAR` (2+ lines)
- Keep existing rules unchanged for other constraint types.
- Update creation path to store all selected participants for n-ary cases.

Acceptance:
- Context menu/manager allow legal n-ary creation.
- Constraint persists and remains editable/selectable.

### Wave 2 — Inspector Relationship Management

#### Task W2-1: Geometry-side linked constraints section
- In entity inspector (geometry entity selected), show linked constraints list:
  - select linked constraint
  - delete linked constraint
  - edit dimensional value when applicable
- Backed by existing relationship source of truth (participant refs + constraint component).

Acceptance:
- Geometry entity exposes usable linked-constraint management without sketch-level list traversal.

#### Task W2-2: Constraint-side linked geometry section
- In entity inspector (constraint entity selected), show participants list:
  - click/select participant geometry
  - optional remove participant action when still legal
- Keep relationship edits consistent with legality policy.

Acceptance:
- Constraint entity supports direct participant navigation/management.

### Wave 3 — Viewport & Workspace UX

#### Task W3-1: Horizontal glyph stack policy
- For geometries with multiple linked constraints:
  - first glyph at base anchor
  - subsequent glyphs offset horizontally to the right (stable ordering by creation/index)
- Maintain dimensional badge alignment per glyph.

Acceptance:
- Glyphs no longer overlap on multi-constraint geometry.

#### Task W3-2: Active sketch workspace window
- Add sketch activation control in SketchManager:
  - activate/deactivate
  - mutually exclusive active sketch
- When active:
  - show dedicated ImGui window containing GeometryManager + ConstraintManager
  - window placement/size persists via normal ImGui settings flow.

Acceptance:
- Only one active sketch at a time.
- Workspace behaves as standalone persistent ImGui window.

#### Task W3-3: Scene hierarchy order adjustment
- Render `Lights` subtree before main scene tree list.

Acceptance:
- Hierarchy ordering matches requested UX.

### Wave 4 — Startup Demo Scene

#### Task W4-1: Seed predefined sketch at startup
- Add opt-in default startup population (current ask: enabled by default):
  - wireframe cube-like profile with arcs for filleted edges
  - straight through-hole represented by circles + 4 connectors
  - representative constraints applied
- Keep scene creation deterministic and isolated in startup helper.

Acceptance:
- Fresh app launch starts with requested sketch/constraints visible and editable.

## Verification Plan

- Build gate (required after each wave):
  - `cmake --build build-vulkan --config Release --target mdCAD`
- Focused manual checks:
  - numeric-only input in manager + popup
  - n-ary parallel/collinear creation
  - participant-preserving deletion behavior
  - no crash on glyph-select+delete and Clear Scene
  - geometry/constraint relationship panes in inspector
  - glyph horizontal stacking behavior
  - active sketch workspace mutual exclusivity + persisted window position
  - lights subtree order
  - startup example sketch appears and is interactive

## Execution Notes

- Implement in wave order; each wave is independently verifiable.
- Keep existing committed Phase 11 fixes intact.
- Prefer extending existing components/contracts over duplicate state.
