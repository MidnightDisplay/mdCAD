---
phase: 51-unsupported-platform-contract
plan: 03
subsystem: control-shell
tags: [avalonia, embed, unsupported-platform, shell, diagnostics]

# Dependency graph
requires:
  - phase: 51-unsupported-platform-contract
    provides: Internal unsupported backend selection, canonical unsupported runtime message, and blocked coordinator behavior
provides:
  - Public control shell wiring through the internal backend factory
  - Immediate unsupported warning precedence and informational-only diagnostic requested-input status lines
  - Automated shell proof for attach-time unsupported state and disabled diagnostic launch behavior
affects: [phase-51, phase-52]

# Tech tracking
tech-stack:
  added: []
  patterns: [factory-wired shell selection, unsupported warning precedence, internal shell attach seam for tests]

key-files:
  created:
    - .planning/phases/51-unsupported-platform-contract/51-03-SUMMARY.md
    - samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs
  modified:
    - samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs

key-decisions:
  - "Reused the existing sealed warning surface and diagnostic status surface instead of adding new unsupported-only chrome, because the current shell already had enough room to tell the truth cleanly."
  - "Made attach-time unsupported presentation synchronous in the control shell so tests and users see the canonical warning immediately while the coordinator reconcile continues in the background."
  - "Kept the new test-only seam internal (`MdCadEmbeddedControl(Func<bool>?)` plus `AttachForTesting()`) so the public control contract stayed unchanged."

patterns-established:
  - "Pattern 1: Let the shared shell compute primary warning truth from backend blocked state first, then treat JSONL/live-refresh as secondary requested intent only in diagnostic mode."
  - "Pattern 2: Prefer internal constructor/hooks plus ordinary control lookups for shell tests instead of introducing a separate headless UI harness."

requirements-completed: [PLAT-01, PLAT-02, PLAT-03]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 51 Plan 03: Unsupported Shell Contract Summary

**Phase 51 is now complete: the public control shell selects supported vs unsupported backends internally, shows the canonical Windows-only unsupported truth immediately, and keeps diagnostic requested-input details secondary without changing the locked Windows runtime path.**

## Performance

- **Duration:** continued from the Phase 51 execution session
- **Completed:** 2026-05-18T14:39:13.7835657+01:00
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Replaced direct `WindowsMdCadEmbedBackend` construction in `MdCadEmbeddedControl` with `MdCadEmbedBackendFactory.Create(...)` while keeping the public constructor and public control API unchanged.
- Added an internal constructor platform-probe seam plus `AttachForTesting()` so the shell can be exercised on the existing Windows-targeted test lane without widening runtime claims.
- Made unsupported warning precedence explicit in the shell: `StartBlockedReason` now outranks snapshot path warnings, and unsupported requested JSONL/live-refresh details stay informational-only in the diagnostic status lines.
- Kept sealed mode minimal by reusing the existing warning surface and inert placeholder, while diagnostic mode now clearly shows `launch: unsupported`, `attach: unsupported`, and a disabled launch button.
- Closed the full control test suite green with the new shell tests included.

## Task Commits

1. **Task 1: Replace direct Windows backend construction with internal factory wiring and unsupported warning precedence** - `9dec4bb` (feat)
2. **Task 2: Keep sealed mode minimal while diagnostic mode exposes secondary unsupported detail and disabled launch affordance** - `9dec4bb` (feat)

## Files Created/Modified

- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` - Now uses internal backend factory selection, blocked-warning precedence, unsupported diagnostic status text, and the internal test seam.
- `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` - Adds direct shell coverage for unsupported attach-time warning, informational-only requested inputs, and disabled diagnostic launch behavior.

## Decisions Made

- No `.axaml` layout changes were required; the existing warning and diagnostic surfaces were already sufficient for the Phase 51 contract once the shell logic changed.
- Unsupported attach/property-change state is refreshed synchronously in the shell, while the background coordinator reconcile remains the source of lifecycle truth.
- The diagnostic requested-input text deliberately ignores Windows-style path warnings on unsupported hosts so the canonical unsupported message remains primary.

## Deviations from Plan

None. The shell closeout stayed inside the planned Phase 51 surface and did not widen the public API or TFM.

## Issues Encountered

- The new shell tests initially hit a namespace shadowing issue around `Control`; constraining the helper with `global::Avalonia.Controls.Control` resolved it cleanly without changing runtime behavior.

## User Setup Required

None.

## Next Phase Readiness

- Phase 51 is complete.
- The next milestone action is to plan Phase 52 so the public control contract can widen to plain `net10.0` without implying cross-platform runtime embedding support.
- Phase 52.1 still needs to be planned before Phase 53 so later Windows proof work can rely on the dotnet-managed runtime refresh path.

---
*Phase: 51-unsupported-platform-contract*
*Completed: 2026-05-18*
