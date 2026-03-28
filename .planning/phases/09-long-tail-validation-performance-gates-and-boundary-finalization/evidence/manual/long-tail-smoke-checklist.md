# Phase 9 Integrated Long-Tail Smoke Checklist (VAL-03)

Use this checklist for the integrated end-to-end manual validation pass required by `VAL-03`.

## Scope and policy

- Scope is limited to required native targets only:
  - macOS Metal
  - Windows Vulkan (MSVC)
- Required workflow chain for each target:
  1. serializer
  2. importer
  3. undo/redo
  4. editor interaction
- Status vocabulary is strict: `PASS`, `FAIL`, `BLOCKED`
- Blocking policy (D-03): any `FAIL` or `BLOCKED` in required workflows blocks Phase 9 closure.

## Preconditions (automation-first)

- Build and gate prerequisites are current:
  - `cmake --build build-vulkan --config Release --target mdcad_math_harness`
  - `cmake --build build-vulkan --config Release --target math-validation`
- Phase evidence references available:
  - Perf evidence: `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/`
  - Boundary evidence: `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/boundary/thin-entrypoint-boundary-finalization.md`
- Report file to populate during execution:
  - `.planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-report.md`

## Platform provenance to capture in report

For each target host, record:

- Host identifier / machine name
- OS version
- GPU backend (Metal or Vulkan)
- Binary path launched
- Commit SHA under test
- Validation operator + timestamp

## Workflow matrix (execute in order)

| ID | Workflow | Target(s) | Required | Report status field |
|---|---|---|---|---|
| LT-VAL03-01 | serializer save/reload roundtrip | macOS + Windows | Yes | `PASS|FAIL|BLOCKED` |
| LT-VAL03-02 | import representative geometry (JSONL + PLY) | macOS + Windows | Yes | `PASS|FAIL|BLOCKED` |
| LT-VAL03-03 | undo/redo after transform and geometry edits | macOS + Windows | Yes | `PASS|FAIL|BLOCKED` |
| LT-VAL03-04 | editor interaction (gizmo + inspector) | macOS + Windows | Yes | `PASS|FAIL|BLOCKED` |

## LT-VAL03-01: serializer workflow

### Steps

1. Open a scene with multiple entities and at least one parent-child chain.
2. Save scene.
3. Reload the same scene.
4. Verify entity count, parent links, transforms (`position`, `rotation`, `scale`) and geometry types remain stable.

### Evidence to capture in report

- Sample scene path(s)
- Observed entity count before/after
- Parent-link check notes
- Transform parity notes
- Geometry type parity notes
- Final status: `PASS`, `FAIL`, or `BLOCKED`

## LT-VAL03-02: import workflow

### Steps

1. Import representative JSONL sample.
2. Import representative PLY point cloud sample.
3. Import representative PLY mesh sample.
4. Verify placement/orientation/scale and expected entity/triangle counts.

### Evidence to capture in report

- Input sample paths
- Placement/orientation/scale outcomes
- Entity/triangle count checks
- Correctness delta note (`No visible delta` or explicit delta)
- Final status: `PASS`, `FAIL`, or `BLOCKED`

## LT-VAL03-03: undo/redo workflow

### Steps

1. Perform transform edits.
2. Perform at least one geometry-affecting edit.
3. Undo back to baseline.
4. Redo to latest state.
5. Confirm no drift or command-sequencing anomalies.

### Evidence to capture in report

- Edited entity IDs and operation notes
- Undo/redo restoration checks
- Any mismatch details
- Final status: `PASS`, `FAIL`, or `BLOCKED`

## LT-VAL03-04: editor interaction workflow

### Steps

1. Drag gizmo axis handle.
2. Drag gizmo plane handle.
3. Edit transform fields via inspector controls.
4. Verify expected viewport behavior and resulting values.

### Evidence to capture in report

- Gizmo axis/plane behavior notes
- Inspector edit lifecycle notes
- Viewport stability notes
- Final status: `PASS`, `FAIL`, or `BLOCKED`

## Exit criteria

- All required rows in the report are filled for both required targets.
- Overall report status is set using strict vocabulary:
  - `PASS` only if every required workflow on every required target is `PASS`
  - otherwise `FAIL` or `BLOCKED` with explicit blocker notes.
