---
phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
plan: 02
subsystem: ui
tags: [avalonia, diagnostics, embed-host, xunit]
requires:
  - phase: 57-01
    provides: native embedded crash summary/log contract on stderr
provides:
  - Avalonia backend crash-detail capture from stderr plus truthful exit fallback
  - Control-owned unexpected-session-loss relay seam for full hosts
  - Avalonia host harness-status mirroring plus dedicated host test lane
affects: [phase-57-validation, avalonia-host, avalonia-mdcad-control]
tech-stack:
  added: []
  patterns: [stderr-derived crash detail relay, control-owned warning persistence, host-only status composition seam]
key-files:
  created: [samples/avalonia-host.tests/AvaloniaHost.Tests.csproj, samples/avalonia-host.tests/MainWindowHarnessStatusTests.cs, samples/avalonia-host.tests/NuGet.Config]
  modified: [samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs, samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs, samples/avalonia-host/MainWindow.axaml.cs, samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs, samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs, .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md]
key-decisions:
  - "Kept the Avalonia backend on the existing stderr plus UnexpectedSessionLoss seam instead of adding new IPC."
  - "Made the control the single warning owner and exposed only a read-only detail property plus event for full hosts."
  - "Added a local NuGet.Config for avalonia-host.tests so the new host lane restores from nuget.org like adjacent Avalonia sample projects."
patterns-established:
  - "Unexpected embedded loss details persist on the sealed control warning surface until the next user-driven launch change."
  - "Full-host status text mirrors the exact observed detail string via a small composition seam instead of scraping control internals."
requirements-completed: [P57-04, P57-05]
duration: 51min
completed: 2026-05-29
---

# Phase 57 Plan 02: Avalonia crash detail relay Summary

**Avalonia backend stderr crash summaries now flow through the sealed control warning surface and into full-host harness status without inventing causes.**

## Performance

- **Duration:** 51 min
- **Started:** 2026-05-29T11:27:14Z
- **Completed:** 2026-05-29T12:17:51Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments
- Captured stable native embedded failure summaries in the Avalonia Windows backend and reused them for attach-loss cleanup paths.
- Added a control-owned unexpected-session-loss relay seam that keeps the sealed warning surface authoritative while exposing host-safe detail mirroring.
- Added Avalonia host tests plus a dedicated host test restore config and marked the 57-02 validation rows green.

## Task Commits

Each task was committed atomically:

1. **Task 1: Capture truthful native failure detail in the Avalonia backend** - `315f65f` (test), `d50983a` (feat)
2. **Task 2: Relay the same detail through the Avalonia control and full host** - `8dc49fe` (test), `feec3f3` (feat)

## Files Created/Modified
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - preserves the last relevant embedded failure line and falls back to truthful exit-state wording.
- `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` - covers startup failure capture, post-attach fatal detail preference, and generic exit fallback.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` - persists unexpected-session-loss detail on the control warning surface and relays it through a read-only property/event seam.
- `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` - proves warning persistence and the new public relay surface.
- `samples/avalonia-host/MainWindow.axaml.cs` - appends mirrored observed detail to harness status via a composition helper.
- `samples/avalonia-host.tests/AvaloniaHost.Tests.csproj` - adds the Avalonia host verification lane.
- `samples/avalonia-host.tests/MainWindowHarnessStatusTests.cs` - verifies harness-status composition mirrors the relayed detail verbatim.
- `samples/avalonia-host.tests/NuGet.Config` - keeps the new host test project on nuget.org-only restore behavior.
- `.planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md` - records 57-02 automated verification as green.

## Decisions Made
- Reused the existing backend stderr/unexpected-session-loss contract to satisfy the threat model’s “truthful detail only” requirement.
- Left warning ownership inside `MdCadEmbeddedControl` and treated the host mirror as a passive observer of the same string.
- Tested host mirroring through a private composition seam instead of constructing a live Avalonia window in unit tests.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed reflection-based control test invocation for nullable arguments**
- **Found during:** Task 2
- **Issue:** The new control test initially selected the wrong private overload and treated `null` as a missing params array.
- **Fix:** Matched private methods by parameter count and passed the nullable warning argument explicitly.
- **Files modified:** `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs`
- **Verification:** `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests"`
- **Committed in:** `feec3f3`

**2. [Rule 3 - Blocking] Added a local NuGet config for the new Avalonia host test project**
- **Found during:** Task 2
- **Issue:** `samples/avalonia-host.tests` inherited a forbidden upstream package source and restore failed with HTTP 403 before the host test lane could run.
- **Fix:** Added `samples/avalonia-host.tests/NuGet.Config` matching the existing Avalonia sample projects so the new lane restores from `nuget.org`.
- **Files modified:** `samples/avalonia-host.tests/NuGet.Config`
- **Verification:** `dotnet test .\samples\avalonia-host.tests\AvaloniaHost.Tests.csproj -c Release --filter "FullyQualifiedName~HarnessStatus|FullyQualifiedName~MainWindow"`
- **Committed in:** `feec3f3`

---

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking)
**Impact on plan:** Both changes were directly required to keep the new verification lanes truthful and runnable. No scope creep.

## Issues Encountered
- A live `MainWindow` test required an Avalonia windowing platform, so the host proof was redirected to the extracted `ComposeHarnessStatus` seam that the plan explicitly allowed.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Avalonia Phase 57 surfaces are ready for the final rebaseline/manual smoke plan.
- Manual Phase 57 host smoke rows remain pending in `57-VALIDATION.md`.

## Self-Check: PASSED
