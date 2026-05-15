---
phase: 48-reusable-avalonia-mdcad-user-control
plan: 02
subsystem: infra
tags: [avalonia, msbuild, runtime, appcontext, embedding]

# Dependency graph
requires:
  - phase: 48-reusable-avalonia-mdcad-user-control
    provides: Windows-only control library foundation and the extracted placeholder HWND seam from Plan 01.
provides:
  - Committed pinned `win-x64` mdCAD runtime bundle under the control project
  - MSBuild content-copy contract that publishes the bundle into `mdcad-runtime/`
  - AppContext-rooted runtime resolver returning both the executable path and runtime root
affects: [phase-48-session-coordinator, avalonia-host, runtime-packaging]

# Tech tracking
tech-stack:
  added: [committed win-x64 mdCAD runtime bundle, AppContext-rooted runtime resolver]
  patterns: [curated runtime refresh script, ProjectReference-driven runtime copy into consumer output]

key-files:
  created:
    - scripts/refresh-pinned-mdcad-runtime.ps1
    - samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe
    - samples/avalonia-mdcad-control/runtime/win-x64/imgui.embedded.ini
    - samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs
  modified:
    - .gitignore
    - samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj
    - samples/avalonia-host/AvaloniaHost.csproj

key-decisions:
  - "Ship a committed curated runtime folder under the control project instead of relying on repo-root `build-vulkan` discovery."
  - "Resolve mdCAD only from `Path.Combine(AppContext.BaseDirectory, \"mdcad-runtime\")` and preserve that folder for later WorkingDirectory use."
  - "Use standard MSBuild content metadata plus ProjectReference consumption instead of custom consumer-side copy scripts."

patterns-established:
  - "Pattern 1: Reusable external-viewer packages own a committed runtime bundle plus a maintainer refresh script rather than rebuilding the native app inside the consuming project."
  - "Pattern 2: Runtime resolution for the reusable control is output-rooted and deterministic; there is no repo-root fallback or caller-supplied runtime override."

requirements-completed: [P48-02]

# Metrics
duration: 8 min
completed: 2026-05-15
---

# Phase 48 Plan 02: Runtime bundle packaging summary

**Pinned `mdcad-runtime` bundle with AppContext-rooted resolution and ProjectReference-driven output copy for the Avalonia sample consumer**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-15T15:31:41.3981797+01:00
- **Completed:** 2026-05-15T15:39:30.9432929+01:00
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Materialized the committed `samples/avalonia-mdcad-control/runtime/win-x64/` bundle with `mdCAD.exe` and `imgui.embedded.ini`, plus a maintainer refresh script that curates the runtime from `build-vulkan/bin/Release`.
- Added the library MSBuild content contract that copies `runtime/win-x64/**` into `mdcad-runtime/` under the consuming app output using standard `Content`, `Link`, `CopyToOutputDirectory`, and `CopyToPublishDirectory` metadata.
- Added `MdCadRuntimeResolver` so later lifecycle code can resolve the copied runtime from `AppContext.BaseDirectory` without any repo-root fallback.
- Switched the sample host project to reference the reusable control library through `ProjectReference`, making it the in-repo proof consumer for the copied runtime bundle.

## Task Commits

Each task was committed atomically:

1. **Task 1: Materialize the pinned runtime bundle and make it source-controlled** - `f110737` (feat)
2. **Task 2: Copy the runtime bundle into consumer output and resolve launches from `mdcad-runtime` only** - `5eaf21f` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified
- `.gitignore` - Allows the committed runtime bundle path through the repo-wide compiled-binary ignore rules.
- `scripts/refresh-pinned-mdcad-runtime.ps1` - Refreshes the curated pinned runtime bundle from `build-vulkan/bin/Release` without becoming part of the consumer build path.
- `samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe` - Committed Windows runtime executable for the reusable control package.
- `samples/avalonia-mdcad-control/runtime/win-x64/imgui.embedded.ini` - Committed embedded-layout seed file packaged with the pinned runtime bundle.
- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` - Copies the pinned runtime bundle into `mdcad-runtime/` under the consuming app output.
- `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs` - Resolves the copied runtime from `AppContext.BaseDirectory` and returns the executable path plus runtime root.
- `samples/avalonia-host/AvaloniaHost.csproj` - References the reusable control library through `ProjectReference` so host builds prove the transitive runtime copy path.

## Decisions Made
- Treated the committed runtime bundle as part of the reusable-control deliverable rather than an implementation detail of the sample harness.
- Kept runtime resolution fixed to `mdcad-runtime/` under the consumer output so later lifecycle code can set `WorkingDirectory` to the copied bundle and preserve embedded layout persistence semantics.
- Reused standard MSBuild content metadata instead of introducing custom host-build scripts for runtime propagation.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `48-03-PLAN.md` to add the managed launch snapshot test seam, serialized session coordinator, and property-driven relaunch logic on top of the new runtime resolver.
- The sample host now receives `mdcad-runtime/mdCAD.exe` via `ProjectReference`, so the lifecycle plan can stop depending on repo-root runtime discovery.

---
*Phase: 48-reusable-avalonia-mdcad-user-control*
*Completed: 2026-05-15*
