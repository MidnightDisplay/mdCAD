---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 02
subsystem: ui
tags: [shared-core, runtime, coordinator, xunit, net10]
requires:
  - phase: 56-01
    provides: Shared launch snapshot, presentation mode, and unsupported-runtime contracts
provides:
  - Shared runtime-path and runtime-resolution contracts for future Avalonia and WPF consumers
  - Framework-neutral session coordinator with relaunch coalescing and explicit-stop semantics
  - Dedicated xUnit proof for canonical runtime-root resolution and shared lifecycle reconciliation
affects: [phase-56-plan-03, phase-56-plan-04, phase-56-plan-05, phase-56-plan-06]
tech-stack:
  added: []
  patterns: [shared non-visual runtime/coordinator services before UI-framework rewiring]
key-files:
  created:
    - samples/mdcad-embed-core/MdCadRuntimePaths.cs
    - samples/mdcad-embed-core/MdCadRuntimeResolver.cs
    - samples/mdcad-embed-core/MdCadSessionCoordinator.cs
    - samples/mdcad-embed-core.tests/MdCadRuntimePathsTests.cs
    - samples/mdcad-embed-core.tests/MdCadRuntimeResolverTests.cs
    - samples/mdcad-embed-core.tests/MdCadSessionCoordinatorTests.cs
  modified: []
key-decisions:
  - Shared runtime resolution now derives the canonical executable and embedded-ini paths from AppContext.BaseDirectory\mdcad-runtime before any UI project is repointed.
  - Relaunch coalescing, explicit stop suppression, and blocked-start behavior now live in MdCad.Embed.Core so Avalonia and WPF can consume one lifecycle truth.
patterns-established:
  - "Shared embed-core services own runtime lookup and reconcile rules; UI stacks stay thin shells over the same lifecycle engine."
  - "Runtime-root and coalesced-relaunch semantics are locked in standalone xUnit tests before backend rewiring."
requirements-completed: [P56-01]
duration: 10 min
completed: 2026-05-28
---

# Phase 56 Plan 02: Move runtime resolution and session coordination into the shared-core lane Summary

**Shared mdCAD runtime lookup and session reconciliation now live in `MdCad.Embed.Core`, with green xUnit proof for canonical `mdcad-runtime` resolution, relaunch coalescing, and explicit-stop behavior.**

## Performance

- **Duration:** 10 min
- **Tasks:** 1
- **Files modified:** 9

## Accomplishments
- Added shared `MdCadRuntimePaths`, `MdCadRuntimeResolver`, and `MdCadSessionCoordinator` types in `samples/mdcad-embed-core/`.
- Ported runtime-resolver and coordinator tests into the shared-core xUnit lane.
- Extended runtime-path proof to include the canonical `imgui.embedded.ini` location under `mdcad-runtime`.

## Task Commits

User requested no commits for this execution, so no task or metadata commits were created.

## Files Created/Modified
- `samples/mdcad-embed-core/MdCadRuntimePaths.cs` - canonical shared runtime-root, executable, and embedded-ini path contract.
- `samples/mdcad-embed-core/MdCadRuntimeResolver.cs` - shared runtime resolver rooted at `AppContext.BaseDirectory\mdcad-runtime`.
- `samples/mdcad-embed-core/MdCadSessionCoordinator.cs` - shared start/stop/reconcile lifecycle coordinator.
- `samples/mdcad-embed-core.tests/MdCadRuntimePathsTests.cs` - runtime-path layout proof including `imgui.embedded.ini`.
- `samples/mdcad-embed-core.tests/MdCadRuntimeResolverTests.cs` - copied-runtime resolution and failure-path tests.
- `samples/mdcad-embed-core.tests/MdCadSessionCoordinatorTests.cs` - relaunch coalescing, explicit-stop, and blocked-start tests.

## Decisions Made
- Kept the shared extraction non-visual so later Avalonia/WPF plans can adopt the lifecycle code without pulling UI-shell concerns into the core assembly.
- Exposed runtime-path details directly from the shared core so both future UI stacks can consume one canonical runtime-root contract.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready to single-source Windows start-info generation and repoint the Avalonia backend to the shared runtime/coordinator types.
- No known blockers from this plan.

## Self-Check: PASSED

- Verified plan-created shared-core runtime/coordinator files exist on disk.
- Automated verification passed: `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadRuntimeResolverTests|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~MdCadRuntimePathsTests"`
- Commit-hash verification was intentionally skipped because this execution honored the user's no-commit request.

---
*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Completed: 2026-05-28*
