# Phase 8 Undo/Editor Targeted Checklist

Use this checklist for focused Phase 8 validation (light gate + mandatory workflow evidence).

## Preconditions

- Build is current for Windows Vulkan path:
  - `cmake --build build-vulkan --config Release --target mdcad_math_harness`
  - `cmake --build build-vulkan --config Release --target math-validation`
- Migrated files are present:
  - `src/math/math_undo_editor.h`
  - `src/undo_redo_exec.h`
  - `src/gizmo/gizmo_vertex_mode.h`
  - `src/ui/ui_entity_inspector.h`

## Undo/redo transform edits

### Setup

- Open app with a scene containing at least one editable entity with a transform component.
- Ensure entity is selected and inspector is visible.

### Steps

1. Change position via inspector drag controls.
2. Change rotation via inspector drag controls.
3. Change scale via inspector drag controls.
4. Trigger undo repeatedly until original transform is restored.
5. Trigger redo repeatedly until latest transform is restored again.

### Expected Outcome

- Position/rotation/scale are restored exactly through undo/redo cycles.
- Viewport updates immediately (no stale transform/render state).
- No command sequencing anomalies (apply/unapply symmetry preserved).

### Observed Result

- PASS/FAIL:
- Notes:
- Correctness Delta Note (if any):

## Gizmo vertex edit workflow

### Setup

- Enter geometry/vertex edit mode on an entity with editable vertices.
- Select one or more vertices.

### Steps

1. Drag selected vertex/vertices using gizmo handles.
2. Observe world-space movement while editing.
3. Trigger undo and confirm prior local positions are restored.
4. Trigger redo and confirm edited positions are restored.

### Expected Outcome

- Vertex movement is stable and consistent with gizmo direction.
- Local-space stored positions update correctly from world-space deltas.
- Undo/redo restores exact pre/post-edit vertex states.

### Observed Result

- PASS/FAIL:
- Notes:
- Correctness Delta Note (if any):

## Inspector edit workflow

### Setup

- Select an editable entity and open inspector sections for transform + geometry.

### Steps

1. Edit transform values (position/rotation/scale) in inspector.
2. Edit geometry values (for applicable geometry type).
3. Confirm command capture occurs at drag end.
4. Undo and redo edits.

### Expected Outcome

- Inspector edits apply immediately and persist in runtime state.
- Undo commands capture old/new values correctly at drag lifecycle boundaries.
- Undo/redo returns entity to expected states without drift.

### Observed Result

- PASS/FAIL:
- Notes:
- Correctness Delta Note (if any):
