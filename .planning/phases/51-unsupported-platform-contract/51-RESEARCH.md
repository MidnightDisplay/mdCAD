# Phase 51: Unsupported-Platform Contract — Research

**Researched:** 2026-05-18  
**Domain:** Avalonia control backend selection + unsupported-runtime contract  
**Confidence:** HIGH

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Unsupported platforms use the same core truthful message in both presentation modes.
- **D-02:** `sealed` stays minimal, while `diagnostic` adds extra status/detail around the same unsupported-platform truth.
- **D-03:** The core message must explain that the host/control is valid, but embedded mdCAD viewing is Windows-only and will not launch on the current platform.
- **D-04:** On unsupported platforms, `StartAsync()` fails immediately with a clear unsupported-runtime error instead of silently succeeding or no-oping.
- **D-05:** The `StartAsync()` failure text should match the same truth shown in the placeholder/warning surface.
- **D-06:** On unsupported platforms, `StopAsync()` is a safe no-op and preserves the unsupported warning/state.
- **D-07:** Unsupported placeholder/state should appear as soon as the control attaches to the visual tree; `AutoStart` never attempts runtime launch there.
- **D-08:** Launch-affecting property changes (`JsonlPath`, live refresh, related launch intent) update descriptive state only on unsupported platforms; they do not trigger launch work.
- **D-09:** In `diagnostic` mode, requested JSONL/live-refresh intent may still be shown as informational-only state on unsupported platforms.
- **D-10:** Unsupported-platform messaging remains the primary warning/detail truth on unsupported hosts.
- **D-11:** Requested JSONL/live-refresh information may appear only as secondary informational diagnostics; normal Windows-oriented JSONL/path validation must not take precedence there.
- **D-12:** In `diagnostic` mode, the control's own launch action is disabled up front when runtime activation is impossible.

### the agent's Discretion
- Choose the exact internal unsupported-backend type/file names and how backend selection is wired, as long as the public control API stays stable and Windows behavior remains untouched.
- Choose the exact visual styling, spacing, and iconography of the unsupported placeholder, as long as `sealed` stays minimal and `diagnostic` remains more explicit.
- Choose the exact exception type used for unsupported `StartAsync()` failure, as long as the failure is immediate and the message matches the locked placeholder truth.
- Choose the exact wording/placement of secondary informational diagnostics for requested JSONL/live-refresh intent, as long as unsupported-platform messaging remains primary.

### Deferred Ideas (OUT OF SCOPE)
- Add a public `IsRuntimeSupported`-style capability property — useful, but not required for Phase 51 and could widen the surface unnecessarily.
- Support host-customizable unsupported placeholder text — valuable polish, but not part of the current contract phase.
- Public TFM widening to plain `net10.0` and the proof host restore/build closure — Phase 52.
- Windows runtime refresh automation via dotnet-managed helper flow — Phase 52.1.
- Consumer proof and docs/onboarding truthfulness expansion — Phases 53 and 54.

## Summary

Current code still constructs `WindowsMdCadEmbedBackend` directly in `MdCadEmbeddedControl`, so non-Windows behavior is effectively a silent idle path: no explicit unsupported message, no real backend selection, and `StartAsync()` currently degrades into a no-op instead of a clear failure. That is the main planning gap.

**Primary recommendation:** add an **internal backend factory** plus an **internal unsupported backend**, keep the existing Windows backend logic intact, and add one small internal "start blocked reason" capability so the shell/coordinator can show the unsupported truth immediately and throw a matching immediate error from `StartAsync()`.

## Phase Requirements

| ID | Description | Research Support |
|---|---|---|
| PLAT-01 | Clear placeholder/warning on non-Windows | Reuse existing warning surface + inert unsupported backend surface; select unsupported backend before Windows launch logic |
| PLAT-02 | AutoStart/property changes never launch on non-Windows | Unsupported backend reports non-startable; coordinator reconcile stays descriptive only |
| PLAT-03 | `StartAsync()` fails clearly; `StopAsync()` safe | Add internal blocked-reason signal; unsupported backend throws immediate standard unsupported exception; stop is no-op |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---|---:|---|---|
| .NET | 10.0.300 SDK present | host/control runtime + tests | already project standard |
| Avalonia | 11.3.15 resolved | control UI | already used by control |
| Avalonia.Desktop | 11.3.15 resolved | desktop host surface | already used by control |

### Supporting
| Library | Version | Purpose | When to Use |
|---|---:|---|---|
| xUnit | 2.5.3 | unit tests | backend/coordinator contract tests |
| Microsoft.NET.Test.Sdk | 17.8.0 | test runner | current test infrastructure |
| coverlet.collector | 6.0.0 | coverage | optional validation |

## Architecture Patterns

### Pattern 1: Internal backend factory
Use a single internal selection seam, not direct `new WindowsMdCadEmbedBackend(...)` in the control.

```csharp
internal static class MdCadEmbedBackendFactory
{
    internal static IMdCadEmbedBackend Create(/* existing callbacks */, Func<bool>? isWindows = null)
    {
        var windows = (isWindows ?? OperatingSystem.IsWindows)();
        return windows
            ? new WindowsMdCadEmbedBackend(/* unchanged Windows wiring */)
            : new UnsupportedMdCadEmbedBackend(/* same shell callbacks */);
    }
}
```

**Why:** lets tests force the unsupported branch on Windows without widening the public TFM.

### Pattern 2: Unsupported backend is inert, explicit, and lifecycle-compatible
Recommended internal contract additions:
- `string? StartBlockedReason { get; }` or equivalent internal capability
- `Surface` = inert Avalonia control
- `CanStartSession = false`
- `HasActiveSession = false`
- `PlaceholderHandle = IntPtr.Zero`
- `StartSessionAsync(...)` throws immediate unsupported exception with canonical message
- `StopSessionAsync(...)` returns completed task
- `RecreateSurfaceAsync(...)` returns completed task

### Pattern 3: Shell owns warning precedence
Do **not** let unsupported hosts surface `MdCadLaunchSnapshot.WarningText` as the primary warning.  
Current snapshot logic is Windows-shaped (`absolute path`, `missing file`, etc.). On unsupported hosts that is misleading.

Use:
1. backend blocked reason as primary warning
2. JSONL/live-refresh only as secondary informational text in diagnostic mode

### Pattern 4: Preserve Windows backend behavior
Do not change:
- runtime resolution
- `--embedded --parent-hwnd`
- attach polling
- resize sync
- relaunch logic

Phase 50 already locked that behavior.

## Don’t Hand-Roll

| Problem | Don’t Build | Use Instead | Why |
|---|---|---|---|
| Unsupported lifecycle | second lifecycle path beside coordinator | existing `MdCadSessionCoordinator` + backend seam | avoids drift |
| Platform detection | ad hoc scattered OS checks | one factory + `OperatingSystem.IsWindows()` | centralized truth |
| Unsupported error semantics | custom silent status-only behavior | immediate standard unsupported exception | matches imperative API expectations |
| Cross-platform proof | fake broad runtime support or early TFM widening | windows-targeted unit tests forcing unsupported backend | proves contract without overclaiming runtime support |

## Common Pitfalls

### 1. Silent idle instead of real unsupported contract
Current likely failure mode. Avoid by selecting unsupported backend explicitly.

### 2. JSONL/path warnings outranking unsupported truth
`MdCadLaunchSnapshot.Create(...)` currently emits Windows-oriented warnings. Unsupported message must win.

### 3. Fixing `StartAsync()` only
If attach-time state and property-change reconcile still use Windows-shaped warnings, the control stays misleading.

### 4. Reopening Windows logic
Phase 51 should wrap around the Windows backend, not refactor it again.

## Code Examples

### Recommended unsupported message constant
Use one canonical string in:
- attach-time warning
- unsupported placeholder
- thrown `StartAsync()` error text

Example:
```csharp
internal const string UnsupportedRuntimeMessage =
    "This host/control is valid, but embedded mdCAD viewing is Windows-only and will not launch on the current platform.";
```

### Recommended `StartAsync()` behavior
Use existing exception flow in `RunCoordinatorTaskAsync(...)`:
- coordinator throws unsupported exception immediately
- control already surfaces `ex.Message` into warning/detail text

That means Phase 51 can reuse current shell error plumbing.

## State of the Art

| Old Approach | Current Recommended Approach | Impact |
|---|---|---|
| Always construct Windows backend and let non-Windows idle implicitly | Select Windows vs unsupported backend explicitly | truthful behavior |
| Unsupported start becomes no-op | Unsupported start throws immediately | stable imperative contract |
| Windows-style snapshot warnings shown everywhere | unsupported warning overrides snapshot warning precedence | avoids misleading UX |

## Open Questions

1. **Exact exception type**
   - Recommendation: `PlatformNotSupportedException`
   - Confidence: MEDIUM

2. **Whether unsupported placeholder text lives only in warning banner or also inside embed surface**
   - Recommendation: reuse existing warning surface as primary truth; unsupported backend surface stays inert/minimal
   - Confidence: MEDIUM-HIGH

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| dotnet | build/test | ✓ | 10.0.300 | — |
| git | workflow | ✓ | 2.51.1.windows.1 | — |
| node | GSD tooling | ✓ | v25.9.0 | — |

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | xUnit 2.5.3 |
| Config file | none |
| Quick run command | `dotnet test .\\samples\\avalonia-mdcad-control.tests\\MdCad.Avalonia.Control.Tests.csproj --no-restore -v minimal` |
| Full suite command | same for Phase 51 scope |

### Current validation state
- Existing unit suite passes: **17/17**
- Current tests already cover:
  - `MdCadSessionCoordinator`
  - `WindowsMdCadEmbedBackend.CreateStartInfo`
  - `MdCadLaunchSnapshot`
  - `MdCadRuntimeResolver`

### Recommended Phase 51 test additions
| Req ID | Test Type | Recommended File |
|---|---|---|
| PLAT-01 | unit | `UnsupportedMdCadEmbedBackendTests.cs` |
| PLAT-02 | unit | `MdCadSessionCoordinatorTests.cs` additions |
| PLAT-03 | unit | `UnsupportedMdCadEmbedBackendTests.cs` + `MdCadSessionCoordinatorTests.cs` |

### Specific test recommendations
1. `StartAsync_WhenBackendUnsupported_ThrowsImmediately_WithCanonicalMessage`
2. `StopAsync_WhenBackendUnsupported_IsSafeNoOp`
3. `AutoStart_WhenBackendUnsupported_NeverInvokesStartDelegate`
4. `LaunchSettingChanges_WhenBackendUnsupported_UpdateInformationalState_WithoutStart`
5. `BackendFactory_SelectsUnsupportedBackend_WhenPlatformProbeFalse`

### What **not** to do in Phase 51 validation
- do not widen the test project to plain `net10.0`
- do not claim real non-Windows runtime proof
- do not add Avalonia headless/UI harness unless Wave 0 proves it is necessary

**Best validation strategy:** keep tests Windows-targeted for now and force unsupported selection through an internal factory/platform-probe seam.

## Sources

### Primary
- `.planning/phases/51-unsupported-platform-contract/51-CONTEXT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/research/FEATURES.md`
- `.planning/research/ARCHITECTURE.md`
- `.planning/research/PITFALLS.md`
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`
- `samples/avalonia-mdcad-control/Host/IMdCadEmbedBackend.cs`
- `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs`
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs`
- `samples/avalonia-mdcad-control.tests/*`

### Official docs
- Microsoft Learn: target frameworks  
  `https://learn.microsoft.com/en-us/dotnet/standard/frameworks`
- Microsoft Learn: platform compatibility analyzer  
  `https://learn.microsoft.com/en-us/dotnet/standard/analyzers/platform-compat-analyzer`
- Microsoft Learn: `OperatingSystem.IsWindows()`  
  `https://learn.microsoft.com/en-us/dotnet/api/system.operatingsystem.iswindows`

## Confidence Assessment

| Area | Level | Reason |
|---|---|---|
| Standard Stack | HIGH | repo packages resolved and test command verified locally |
| Architecture | HIGH | directly constrained by Phase 50/51 context and current code seam |
| Pitfalls | HIGH | current code shows silent-idle and Windows-shaped warning precedence risks |
| Exact exception choice | MEDIUM | convention-based recommendation |

## RESEARCH COMPLETE

**Phase:** 51 - Unsupported-Platform Contract  
**Confidence:** HIGH

### Key Findings
- Current non-Windows behavior is mainly **implicit idle**, not an explicit unsupported contract.
- The safest plan is **factory-selected internal unsupported backend**, not another public API or public TFM change.
- `StartAsync()` needs a **separate immediate unsupported failure path**; current coordinator logic otherwise no-ops.
- Unsupported warning text must **override Windows-oriented JSONL/path warnings**.
- Validation should stay **unit-test based** in the existing Windows-targeted test project by forcing the unsupported branch internally.

### Ready for Planning
Yes. The planner should build around:
1. backend factory,
2. unsupported backend,
3. blocked-reason/immediate-failure seam,
4. warning-precedence fix,
5. unit-test-only validation for this phase.
