# Phase 9 Integrated Long-Tail Smoke Report (VAL-03)

## Plan Traceability

- Phase: `09-long-tail-validation-performance-gates-and-boundary-finalization`
- Plan: `09-03`
- Requirement: `VAL-03`
- Checklist: `evidence/manual/long-tail-smoke-checklist.md`

## Execution provenance

| Field | Value |
|---|---|
| Report updated at (UTC) | 2026-03-28T13:14:09Z |
| Commit under test | `ee825f5` |
| Operator | `GSD executor` |
| Required targets | `macOS Metal`, `Windows Vulkan (MSVC)` |

## Target host records

### Windows Vulkan (MSVC)

- host: current executor host (Windows)
- backend: Vulkan
- binary path: `.\build-vulkan\bin\Release\mdCAD.exe`
- status: `PENDING MANUAL EXECUTION`

### macOS Metal

- host: not available in current executor environment
- backend: Metal
- binary path: `./build/bin/mdCAD`
- status: `BLOCKED (host unavailable from current session)`

## Workflow outcomes (required coverage)

| ID | Workflow | Windows Vulkan | macOS Metal | Notes |
|---|---|---|---|---|
| LT-VAL03-01 | serializer | BLOCKED | BLOCKED | Manual app workflow not executed in this automated session. |
| LT-VAL03-02 | import | BLOCKED | BLOCKED | Manual app workflow not executed in this automated session. |
| LT-VAL03-03 | undo | BLOCKED | BLOCKED | Manual app workflow not executed in this automated session. |
| LT-VAL03-04 | editor | BLOCKED | BLOCKED | Manual app workflow not executed in this automated session. |

## Blockers

1. Manual native workflow execution requires human verification in live app sessions.
2. macOS Metal target execution is unavailable from this Windows-only environment.

## Overall status

`BLOCKED`

Per D-03 policy, unresolved manual workflow statuses block Phase 9 completion.
