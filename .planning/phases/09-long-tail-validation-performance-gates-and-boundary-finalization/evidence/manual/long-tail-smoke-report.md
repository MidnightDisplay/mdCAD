# Phase 9 Integrated Long-Tail Smoke Report (VAL-03)

## Plan Traceability

- Phase: `09-long-tail-validation-performance-gates-and-boundary-finalization`
- Plan: `09-03`
- Requirement: `VAL-03`
- Checklist: `evidence/manual/long-tail-smoke-checklist.md`

## Execution provenance

| Field | Value |
|---|---|
| Report updated at (UTC) | 2026-03-30T10:51:56Z |
| Commit under test | `4f06080` |
| Operator | `GSD executor` |
| Required targets | `macOS Metal`, `Windows Vulkan (MSVC)` |

## Target host records

### Windows Vulkan (MSVC)

- host: current executor host (Windows)
- backend: Vulkan
- binary path: `.\build-vulkan\bin\Release\mdCAD.exe`
- status: `MANUAL EXECUTION COMPLETE`

### macOS Metal

- host: not available in current executor environment
- backend: Metal
- binary path: `./build/bin/mdCAD`
- status: `BLOCKED (host unavailable from current session)`

## Workflow outcomes (required coverage)

| ID | Workflow | Windows Vulkan | macOS Metal | Notes |
|---|---|---|---|---|
| LT-VAL03-01 | serializer | PASS | BLOCKED | Windows manual run passed serializer checklist items. macOS host unavailable in current session. |
| LT-VAL03-02 | import | FAIL | BLOCKED | Regression: PLY point cloud import fails for `C:\dev\pc1_Wednesday, 17 December 2025 at 15_08_15 Greenwich Mean Time.ply` after "points parsed". Control sample `C:\dev\1m.ply` imports successfully. |
| LT-VAL03-03 | undo | PASS | BLOCKED | Windows manual run passed undo/redo checklist items. macOS host unavailable in current session. |
| LT-VAL03-04 | editor | PASS | BLOCKED | Windows manual run passed gizmo/inspector editor checklist items. macOS host unavailable in current session. |

## Blockers

1. **LT-VAL03-02 import regression (required workflow FAIL):** PLY point cloud import fails for one representative sample after "points parsed" (`C:\dev\pc1_Wednesday, 17 December 2025 at 15_08_15 Greenwich Mean Time.ply`), while `C:\dev\1m.ply` succeeds.
2. macOS Metal target execution is unavailable from this Windows-only environment, so required macOS VAL-03 rows remain `BLOCKED`.

## Issue notes (required failures)

- **LT-VAL03-02 (import):** `FAIL` on Windows Vulkan due to sample-specific PLY point-cloud regression after parser stage; this is a required manual workflow and blocks closure per D-03.
- **Status semantics applied:** `PASS` only where validated by manual run, `FAIL` for observed regression, `BLOCKED` where host execution was unavailable.

## Overall status

`FAIL (BLOCKING)`

Per D-03 policy, any unresolved required `FAIL` or `BLOCKED` status blocks Phase 9 completion.
