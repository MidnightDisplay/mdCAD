# Phase 9 Integrated Long-Tail Smoke Report (VAL-03)

## Plan Traceability

- Phase: `09-long-tail-validation-performance-gates-and-boundary-finalization`
- Plan: `09-03`
- Requirement: `VAL-03`
- Checklist: `evidence/manual/long-tail-smoke-checklist.md`

## Execution provenance

| Field | Value |
|---|---|
| Report updated at (UTC) | 2026-03-30T14:25:00Z |
| Commit under test | `3a96d9b` |
| Operator | `GSD executor` |
| Required targets | `macOS Metal`, `Windows Vulkan (MSVC)` |

## Target host records

### Windows Vulkan (MSVC)

- host: current executor host (Windows)
- backend: Vulkan
- binary path: `.\build-vulkan\bin\Release\mdCAD.exe`
- status: `MANUAL EXECUTION COMPLETE`

### macOS Metal

- host: operator-reported native macOS host
- backend: Metal
- binary path: `./build/bin/mdCAD`
- status: `MANUAL EXECUTION COMPLETE (operator attested)`

## Workflow outcomes (required coverage)

| ID | Workflow | Windows Vulkan | macOS Metal | Notes |
|---|---|---|---|---|
| LT-VAL03-01 | serializer | PASS | PASS | Windows manual run passed serializer checklist items. macOS run reported as PASS by operator attestation. |
| LT-VAL03-02 | import | PASS | PASS | Windows retest outcome (user-reported after commit `e6c3bb8`): PASS for required sample `C:\dev\pc1_Wednesday, 17 December 2025 at 15_08_15 Greenwich Mean Time.ply` in both single-node point-cloud and editable individual-selectable-entities modes; control sample `C:\dev\1m.ply` also PASS. macOS checklist execution reported PASS by operator attestation. |
| LT-VAL03-03 | undo | PASS | PASS | Windows manual run passed undo/redo checklist items. macOS checklist execution reported PASS by operator attestation. |
| LT-VAL03-04 | editor | PASS | PASS | Windows manual run passed gizmo/inspector editor checklist items. macOS checklist execution reported PASS by operator attestation. |

## Blockers

None currently recorded for VAL-03.

## Issue notes (required failures)

- **LT-VAL03-02 (import):** `PASS` on Windows Vulkan from user-provided manual retest evidence after commit `e6c3bb8`; both required sample paths (single-node + editable individual selectable entities) and control sample succeeded.
- **Provenance note:** This report consumes operator-provided validation attestation in-session: "successfully ran through all the checklist validations - works as intended."
- **Status semantics applied:** All required workflows now recorded as PASS on both required native targets.

## Overall status

`PASS`

Per D-03 policy, all required rows are PASS; no unresolved FAIL/BLOCKED statuses remain in VAL-03 evidence.
