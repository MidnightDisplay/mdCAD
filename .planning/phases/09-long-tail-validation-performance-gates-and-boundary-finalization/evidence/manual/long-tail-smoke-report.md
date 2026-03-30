# Phase 9 Integrated Long-Tail Smoke Report (VAL-03)

## Plan Traceability

- Phase: `09-long-tail-validation-performance-gates-and-boundary-finalization`
- Plan: `09-03`
- Requirement: `VAL-03`
- Checklist: `evidence/manual/long-tail-smoke-checklist.md`

## Execution provenance

| Field | Value |
|---|---|
| Report updated at (UTC) | 2026-03-30T11:10:00Z |
| Commit under test | `457057d` |
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
| LT-VAL03-02 | import | FAIL | BLOCKED | Windows split outcome after Task 1 fix: PASS when importing `C:\dev\pc1_Wednesday, 17 December 2025 at 15_08_15 Greenwich Mean Time.ply` as single-node point cloud, FAIL when importing as individual selectable entities; control sample `C:\dev\1m.ply` remains successful. |
| LT-VAL03-03 | undo | PASS | BLOCKED | Windows manual run passed undo/redo checklist items. macOS host unavailable in current session. |
| LT-VAL03-04 | editor | PASS | BLOCKED | Windows manual run passed gizmo/inspector editor checklist items. macOS host unavailable in current session. |

## Blockers

1. **LT-VAL03-02 import regression (required workflow FAIL):** For `C:\dev\pc1_Wednesday, 17 December 2025 at 15_08_15 Greenwich Mean Time.ply`, point-cloud node path now passes, but editable individual-entities path still fails; control `C:\dev\1m.ply` succeeds.
2. macOS Metal target execution is unavailable from this Windows-only environment, so required macOS VAL-03 rows remain `BLOCKED`.

## Issue notes (required failures)

- **LT-VAL03-02 (import):** `FAIL` on Windows Vulkan due to remaining failure in the editable individual selectable entities path for the required sample; single-node point cloud path passes.
- **Remediation applied (pending manual re-check):** importer parse-completion gating was aligned for editable imports to finalize when vertex parsing is complete for point-cloud workflows; requires rerun of LT-VAL03-02 editable path on required targets before status can move from `FAIL`.
- **Status semantics applied:** `PASS` only where validated by manual run, `FAIL` for observed regression, `BLOCKED` where host execution was unavailable.

## Overall status

`FAIL (BLOCKING)`

Per D-03 policy, any unresolved required `FAIL` or `BLOCKED` status blocks Phase 9 completion.
