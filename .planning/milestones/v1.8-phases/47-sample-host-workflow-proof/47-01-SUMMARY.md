---
phase: 47-sample-host-workflow-proof
plan: 01
subsystem: avalonia-host
tags: [host, avalonia, jsonl, embedded, launch]

# Dependency graph
requires:
  - phase: 47-sample-host-workflow-proof
    provides: Research, validation, and execution plan for the sample host proof workflow
provides:
  - Bundled example JSONL content copied into the deployed host output
  - Absolute bundled-example resolution from `AppContext.BaseDirectory`
  - Existing mdCAD CLI launch contract wired with `--jsonl` and optional `--jsonl-live-refresh`
affects: [47-02, host-launch-proof, bundled-example-flow]

# Tech tracking
tech-stack:
  added: []
  patterns: [content-copy example fixture, host-owned absolute path proof, explicit live-refresh pass-through]

key-files:
  created:
    - samples/avalonia-host/resources/examples/sample-host-proof.jsonl
    - .planning/phases/47-sample-host-workflow-proof/47-01-SUMMARY.md
  modified:
    - samples/avalonia-host/AvaloniaHost.csproj
    - samples/avalonia-host/MainWindow.axaml.cs

key-decisions:
  - "The bundled JSONL proof file is copied beside the built host as normal content, not embedded as a managed resource or extracted to a temp file."
  - "The host resolves the bundled example from `AppContext.BaseDirectory`, which keeps launch proof tied to the deployed app instead of repo-relative assumptions."
  - "The sample host now accepts `--jsonl-live-refresh` as a host-side explicit opt-in and forwards it to mdCAD without changing default-off behavior."

patterns-established:
  - "Pattern 1: Host-side proof fixtures that mdCAD must open by path should live under `resources/examples` and be copied to output."
  - "Pattern 2: Host-visible pre-launch detail text should report the exact absolute bundled-example path being requested whenever HOST-02 is in scope."

requirements-completed: [HOST-02]

# Metrics
duration: continued-session
completed: 2026-05-15
---

# Phase 47 Plan 01: Bundled Example Launch Contract Summary

**The Avalonia sample host now ships with a real bundled JSONL proof file, resolves it from the deployed host output, and launches mdCAD with `--jsonl <absolute-path>` plus optional explicit live refresh on the restored MSVC Vulkan path**

## Performance

- **Duration:** continued from the Phase 47 execution session after planning completed
- **Started:** once `build-vulkan` was restored from the temporary MinGW tree back to the MSVC Vulkan generator
- **Completed:** 2026-05-15T11:39:58.2355004+01:00
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Added `samples/avalonia-host/resources/examples/sample-host-proof.jsonl` as a tiny valid flat JSONL proof fixture using the same point-element shape already covered by repo tests.
- Updated `samples/avalonia-host/AvaloniaHost.csproj` so `resources/examples/**` is copied to the deployed host output with `PreserveNewest`.
- Extended `HostLaunchOptions` in `samples/avalonia-host/MainWindow.axaml.cs` with a host-side explicit `--jsonl-live-refresh` opt-in that stays default-off unless requested.
- Added bundled-example resolution through `Path.Combine(AppContext.BaseDirectory, "resources", "examples", "sample-host-proof.jsonl")` and `Path.GetFullPath(...)`, with an in-host failure path before process launch if the file is missing.
- Switched the mdCAD launch process to pass `--jsonl <absolute-path>` and optional `--jsonl-live-refresh` via `ProcessStartInfo.ArgumentList`, avoiding path-quoting problems.
- Surfaced the resolved absolute example path in host-owned pre-launch status/detail text so HOST-02 can be proven without any new IPC.
- Restored `build-vulkan` to the MSVC Vulkan generator before validation and verified 47-01 on that lane with the host build, bundled-example copy check, and the targeted native tests.

## Task Commits

Each task was committed atomically where practical:

1. **Task 1: Add bundled example fixture and copy-to-output contract** - pending
2. **Task 2: Resolve bundled example and append startup JSONL launch flags** - pending

## Files Created/Modified
- `samples/avalonia-host/resources/examples/sample-host-proof.jsonl` - Bundled JSONL proof fixture copied beside the host build output.
- `samples/avalonia-host/AvaloniaHost.csproj` - Copies `resources/examples/**` into the deployed host output.
- `samples/avalonia-host/MainWindow.axaml.cs` - Resolves the bundled example, forwards `--jsonl` and optional `--jsonl-live-refresh`, and reports the resolved absolute path before launch.

## Decisions Made
- Used a host CLI flag for early live-refresh opt-in scaffolding so 47-01 can stay within `MainWindow.axaml.cs` while 47-02 adds the actual checkbox control.
- Treated restoration of the MSVC Vulkan `build-vulkan` tree as part of 47-01 readiness because the user explicitly asked future validation to return to that lane.

## Deviations from Plan

- None.

## Issues Encountered

- Phase 46 had left `build-vulkan` configured as a MinGW tree. Before validating 47-01, the build directory was recreated with the `Visual Studio 18 2026` generator so host/native verification returned to the intended MSVC Vulkan path.

## User Setup Required

None - the bundled example now travels with the host output, and optional startup live refresh remains an explicit host-side opt-in.

## Next Phase Readiness
- 47-02 can now focus on the host UI/session controller and repeated lifecycle proof because the bundled example and launch contract are already in place.
- The future manual checklist can rely on a stable deployed example path instead of ad hoc file setup.

---
*Phase: 47-sample-host-workflow-proof*
*Completed: 2026-05-15*
