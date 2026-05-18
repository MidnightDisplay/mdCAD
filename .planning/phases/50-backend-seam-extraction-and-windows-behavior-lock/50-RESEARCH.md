# Phase 50: Backend Seam Extraction and Windows Behavior Lock - Research

**Researched:** 2026-05-18
**Domain:** Avalonia embedded-control internal backend extraction for Windows child-HWND hosting
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Phase 50 stays a behavior-preserving refactor. It must not widen TFMs, land non-Windows placeholder behavior, or add new user-facing capabilities early.
- **D-02:** The current public control API stays unchanged in this phase; the work is about internal ownership and seam extraction only.
- **D-03:** Keep one public control project/assembly and extract an **internal** backend seam rather than splitting the public consumer surface or package layout in Phase 50.
- **D-04:** The backend seam should make Windows runtime ownership more explicit, not less; the goal is to isolate Win32/runtime logic from the shared control shell while keeping the runtime contract library-owned.
- **D-05:** Phase 50 must explicitly preserve the current Windows runtime bundle lookup and `--embedded --parent-hwnd` process-launch path.
- **D-06:** Phase 50 done bar includes Windows proof that the diagnostic harness still launches, attaches, stops, and relaunches through the existing child-HWND workflow.
- **D-07:** Sealed and diagnostic surfaces must remain functionally equivalent during the extraction; any user-visible behavior change belongs in later phases.

### the agent's Discretion
- Choose the exact internal backend interface/class names and file layout, provided D-01 through D-07 remain true.
- Choose whether `MdCadLaunchSnapshot` stays as-is or gets split into a more backend-friendly shared request + Windows-specific launch shape, provided the public control API stays unchanged in this phase.
- Choose the exact test split between backend-neutral and Windows-specific proof, provided the Windows runtime seam stays explicitly verified.

### Deferred Ideas (OUT OF SCOPE)
- Explicit unsupported-platform placeholder/no-launch behavior — Phase 51.
- Plain `net10.0` public TFM widening and consumer restore/build proof — Phase 52.
- Plain-host consumer proof plus Windows regression closure expansion — Phase 53.
- Docs/onboarding truthfulness cleanup — Phase 54.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WPRS-01 | On Windows, the control still resolves `mdcad-runtime\mdCAD.exe` from consumer output and launches mdCAD with the existing child-HWND contract. | Keep `MdCadRuntimeResolver` semantics unchanged, move launch/runtime lookup behind one Windows backend, and preserve `ProcessStartInfo.ArgumentList` + working directory behavior. |
| WPRS-02 | On Windows, sealed and diagnostic modes preserve current attach/relaunch/status behavior after the host-compatibility widening. | Keep `MdCadEmbeddedControl` as the shared shell for public properties + presentation, keep `MdCadSessionCoordinator` as the relaunch gate, and preserve the current diagnostic harness as the regression authority. |
</phase_requirements>

## Summary

The current seam problem is concrete: `MdCadEmbeddedControl.axaml.cs` is simultaneously the public Avalonia shell, the diagnostic/sealed presentation owner, the Win32 placeholder owner, the runtime resolver caller, the process launcher, the attach poller, the resize synchronizer, and the teardown/relaunch manager. That mixes platform-neutral UI ownership with Windows-only runtime mechanics in one class.

Phase 50 should extract exactly one **internal Windows backend boundary** while leaving the public control shape, assembly, TFM, and runtime behavior untouched. The safest shape is: keep `MdCadEmbeddedControl` as a thin shared shell, keep `MdCadSessionCoordinator` as the backend-neutral lifecycle gate, and move placeholder HWND creation, runtime lookup, process launch, attach polling, resize sync, and process/child cleanup into a Windows backend implementation.

**Primary recommendation:** Introduce an internal `IMdCadEmbedBackend` plus a single `WindowsMdCadEmbedBackend` now; do **not** add an unsupported backend, public factory, or TFM change until later phases.

## Standard Stack

### Core
| Library / Asset | Version | Purpose | Why Standard |
|-----------------|---------|---------|--------------|
| .NET SDK | 10.0.300 installed | Build/test the Avalonia control and hosts | Matches repo net10 work and is available locally |
| `MdCad.Avalonia.Control` | repo project, `net10.0-windows10.0.19041.0` in Phase 50 | Single public control assembly | Locked by phase constraints: no public split, no TFM widening yet |
| Avalonia | repo uses `11.3.*` (NuGet line currently reaches `11.3.15`) | Shared host UI/control surface | Already proven in v1.8/v1.9 prep; upgrade would add avoidable behavior drift |

### Supporting
| Library / Asset | Version | Purpose | When to Use |
|-----------------|---------|---------|-------------|
| `Avalonia.Desktop` | repo uses `11.3.*` | Desktop host plumbing | Keep unchanged in Phase 50 |
| `Microsoft.NET.Test.Sdk` | repo pinned `17.8.0` (registry newer exists) | Test execution | Preserve existing test stack; do not upgrade during behavior lock |
| `xunit` | repo pinned `2.5.3` (registry newer exists) | Unit tests for launch snapshot/coordinator | Extend only with seam-preservation tests |
| repo runtime bundle | `samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe` | Windows runtime payload copied to `mdcad-runtime\...` | Preserve exact copied-runtime contract |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Internal seam in one public assembly | Split public Windows/non-Windows assemblies | Violates locked Phase 50 scope and adds packaging churn early |
| Direct `WindowsMdCadEmbedBackend` instantiation in the control | Factory + multiple backends now | Adds unsupported-platform behavior work too early; better deferred to Phase 51 |
| Keep `MdCadLaunchSnapshot` unchanged in Phase 50 | Split into request + Windows launch shape now | Possible, but increases refactor surface; only do it if backend boundary becomes awkward |

**Installation:** No package or TFM changes recommended in Phase 50.

## Architecture Patterns

### Recommended Project Structure
```text
samples/avalonia-mdcad-control/
├── MdCadEmbeddedControl.axaml          # Shared warning/diagnostic/sealed UI
├── MdCadEmbeddedControl.axaml.cs       # Shared shell + coordinator wiring only
└── Host/
    ├── IMdCadEmbedBackend.cs           # Internal backend contract
    ├── MdCadSessionCoordinator.cs      # Backend-neutral lifecycle gate
    ├── MdCadLaunchSnapshot.cs          # Shared launch snapshot (keep for Phase 50 unless forced)
    └── Windows/
        ├── WindowsMdCadEmbedBackend.cs # Win32/runtime/process/attach implementation
        ├── EmbedNativeControlHost.cs   # Windows placeholder surface
        ├── MdCadRuntimeResolver.cs     # Exact AppContext.BaseDirectory\\mdcad-runtime contract
        └── Win32NativeMethods.cs       # P/Invoke helpers extracted from host helper
```

### Current seam/problem
`MdCadEmbeddedControl.axaml.cs` currently owns:
- public StyledProperty surface
- warning and diagnostic UI state
- `DispatcherTimer` attach/resize loops
- placeholder handle storage
- runtime bundle resolution
- `mdCAD.exe --embedded --parent-hwnd` process launch
- child attach discovery via `EnumChildWindows`
- resize sync via `MoveWindow`
- graceful/forced stop and relaunch surface recreation

That is the exact behavior Phase 50 must preserve, but it should no longer all live in the shared shell.

### Ownership split for Phase 50 only

| Responsibility | Shared shell keeps it | Move to Windows backend |
|----------------|-----------------------|-------------------------|
| Public API (`JsonlPath`, `StartupLiveRefreshEnabled`, `AutoStart`, `PresentationMode`, `StartAsync`, `StopAsync`) | Yes | No |
| Sealed vs diagnostic UI surface | Yes | No |
| Warning text display + host-owned status text blocks | Yes | No |
| Launch snapshot capture from public properties | Yes | No |
| Lifecycle gating / coalesced relaunch decisions | `MdCadSessionCoordinator` | No |
| Native placeholder `NativeControlHost` creation/destruction | No | Yes |
| HWND readiness and size/validity checks | No | Yes |
| Runtime lookup under `AppContext.BaseDirectory\\mdcad-runtime` | No | Yes |
| `ProcessStartInfo` construction and launch arguments | No | Yes |
| stdout/stderr failure capture for embedded startup errors | No | Yes |
| Child attach polling and timeout handling | No | Yes |
| Child resize sync and parent/child health checks | No | Yes |
| Graceful exit / fallback kill / surface recreation details | No | Yes |

### Pattern 1: Thin shared shell + single Windows backend
**What:** Public control remains the only consumer entrypoint, but delegates Windows-only work to an internal backend.
**When to use:** Immediately in Phase 50.
**Example:**
```csharp
internal interface IMdCadEmbedBackend
{
    Control Surface { get; }
    bool CanStartSession { get; }
    event Action StateChanged;
    Task StartAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken);
    Task StopAsync(CancellationToken cancellationToken);
    Task RecreateSurfaceAsync(CancellationToken cancellationToken);
}
```
Source basis: repo architecture recommendation from `.planning/research/ARCHITECTURE.md` and current shell/backend split pressure visible in `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`.

### Pattern 2: Keep `MdCadSessionCoordinator` backend-neutral
**What:** Coordinator decides *when* to start/stop/recreate; backend decides *how* Windows work happens.
**When to use:** For all launch-affecting property changes and attach/relaunch behavior.
**Example:**
```csharp
bool launchChanged = _isSessionRunning && (!_activeSnapshot.HasValue || _activeSnapshot.Value != snapshot);

if (_isSessionRunning && (!shouldBeRunning || launchChanged))
{
    await _stopSessionAsync(CancellationToken.None);
    _isSessionRunning = false;
    _activeSnapshot = null;
    await _recreateSurfaceAsync(CancellationToken.None);
}
```
Source basis: existing coordinator behavior in `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs`.

### Pattern 3: Preserve host-owned diagnostic truth
**What:** Keep warning/status text in the control and keep `samples/avalonia-host/MainWindow.axaml.cs` as the external workflow proof surface.
**When to use:** Throughout extraction and regression validation.
**Example:** Continue setting `PresentationMode`, `AutoStart`, `StartupLiveRefreshEnabled`, and `JsonlPath` from `MainWindow.axaml.cs` exactly as today; only internal routing changes.

### Anti-Patterns to Avoid
- **Backend factory explosion in Phase 50:** Adding unsupported backend selection now pulls Phase 51 into this refactor.
- **Public API leakage of HWND/process types:** Violates the platform-neutral public surface goal.
- **Coordinator duplication:** Do not create a second relaunch controller inside the backend.
- **Behavioral cleanup “while you are here”:** Any visible diagnostic/sealed change breaks the phase contract.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Relaunch coalescing | A new ad hoc restart manager inside the backend | Existing `MdCadSessionCoordinator` | Already handles generation coalescing, auto-start suppression, and recreate-before-restart |
| Runtime discovery | New repo-root probing or configurable runtime path system | Existing `MdCadRuntimeResolver.Resolve()` contract | Current copied-bundle lookup is locked and already consumer-output based |
| Launch argument composition | Alternative embed protocol or shell-quoted arg string | Existing `ProcessStartInfo.ArgumentList` shape | Preserves exact `--embedded --parent-hwnd` contract and avoids quoting regressions |
| Child attach strategy | New IPC/handshake layer | Existing child-HWND detection via `EnumChildWindows` + parent validation | Proven in v1.8 harness and explicitly in scope to preserve |
| Regression proof surface | New proof harness | Existing Windows diagnostic host + current control tests | Lowest-risk path to behavior-lock evidence |

**Key insight:** Phase 50 is not the place to improve the embedding model; it is the place to isolate the already-proven one.

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — verified this phase reorganizes in-process control/backend ownership only; no persisted keys, collections, or IDs are renamed or migrated. | None |
| Live service config | None — no external service/UI-managed config owns this seam. | None |
| OS-registered state | None — no task/service/unit registration names change; runtime still launches `mdCAD.exe` with the same embed args. | None |
| Secrets/env vars | None — no secret names or env-var contracts participate in the extraction. | None |
| Build artifacts | Existing copied runtime artifact contract: `mdcad-runtime\\mdCAD.exe` sourced from `samples/avalonia-mdcad-control/runtime/win-x64/`. | Code edit only: preserve folder/executable naming and add regression proof; no data migration |

## Common Pitfalls

### Pitfall 1: Accidentally changing the launch contract during extraction
**What goes wrong:** Backend extraction silently changes working directory, omits `--embedded`, changes `--parent-hwnd` formatting, or alters optional JSONL/live-refresh pass-through.
**Why it happens:** Launch code moves files and gets “cleaned up.”
**How to avoid:** Treat current `ProcessStartInfo` fields and `ArgumentList` order/contents as locked behavior.
**Warning signs:** Harness still builds but mdCAD exits before attach or status text changes from current expectations.

### Pitfall 2: Breaking placeholder destruction/recreation ordering
**What goes wrong:** Relaunch no longer recreates the native surface before the next start, or cleanup destroys the wrong placeholder at the wrong time.
**Why it happens:** Placeholder lifecycle is currently interleaved with stop/recreate logic in the control.
**How to avoid:** Move the entire placeholder lifecycle together into the backend and keep coordinator stop → recreate → possible restart order intact.
**Warning signs:** Second launch never attaches, old child HWND stays parented, or resize sync stops after relaunch.

### Pitfall 3: Smuggling Phase 51 into Phase 50
**What goes wrong:** Extraction starts adding explicit non-Windows behavior, capability APIs, or backend selection UX.
**Why it happens:** An interface invites premature multi-backend implementation.
**How to avoid:** Instantiate only the Windows backend in Phase 50 and defer unsupported behavior to Phase 51.
**Warning signs:** New public members, new warning text contract, or any non-Windows-specific tests appear in this phase.

### Pitfall 4: Changing sealed vs diagnostic behavior by accident
**What goes wrong:** Diagnostic buttons/status move into the backend, sealed mode warning visibility changes, or host-facing text diverges.
**Why it happens:** UI and lifecycle logic are currently intertwined.
**How to avoid:** Keep all presentation decisions in the shared shell and route only behavior into the backend.
**Warning signs:** `samples/avalonia-host/MainWindow.axaml.cs` no longer exercises the same control surface as before.

### Pitfall 5: Over-splitting `MdCadLaunchSnapshot`
**What goes wrong:** Warning precedence or JSONL/live-refresh semantics change during request/launch-type refactoring.
**Why it happens:** Snapshot looks Windows-shaped, so it is tempting to redesign it immediately.
**How to avoid:** Keep it as-is in Phase 50 unless the backend seam cannot be expressed cleanly otherwise.
**Warning signs:** Existing `MdCadLaunchSnapshotTests` need semantic rewrites instead of simple relocation/extension.

## Code Examples

Verified repo patterns to preserve:

### Windows launch contract
```csharp
ProcessStartInfo startInfo = new()
{
    FileName = runtime.ExecutablePath,
    UseShellExecute = false,
    RedirectStandardOutput = true,
    RedirectStandardError = true,
    CreateNoWindow = false,
    WorkingDirectory = runtime.RuntimeRoot,
};
startInfo.ArgumentList.Add("--embedded");
startInfo.ArgumentList.Add("--parent-hwnd");
startInfo.ArgumentList.Add($"0x{placeholderHandle.ToInt64():X}");
```
Source: `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`

### Windows runtime bundle lookup
```csharp
string runtimeRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "mdcad-runtime"));
string executablePath = Path.Combine(runtimeRoot, "mdCAD.exe");
```
Source: `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs`

### Relaunch coalescing gate
```csharp
bool launchChanged = _isSessionRunning && (!_activeSnapshot.HasValue || _activeSnapshot.Value != snapshot);

if (_isSessionRunning && (!shouldBeRunning || launchChanged))
{
    await _stopSessionAsync(CancellationToken.None);
    _isSessionRunning = false;
    _activeSnapshot = null;
    await _recreateSurfaceAsync(CancellationToken.None);
}
```
Source: `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs`

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Public control directly owns Win32/runtime/process details | Shared shell delegates to internal backend while preserving the same Windows runtime contract | Phase 50 target | Enables later unsupported-platform and TFM work without implying behavior changes now |
| Repo-root/runtime-path flexibility | Fixed `AppContext.BaseDirectory\\mdcad-runtime` consumer-output lookup | Phase 48 | Must remain unchanged in Phase 50 |
| Sample host as one-off proof surface | Sample host + reusable control diagnostic mode as regression authority | Phases 47-49 | Keep this proof path intact during extraction |

**Deprecated/outdated for this phase:**
- Reintroducing repo-root runtime probing: rejected by Phase 48 and contrary to current shipped behavior.
- Splitting the public assembly now: deferred beyond Phase 50 and not needed for milestone success.

## Open Questions

1. **Should `MdCadLaunchSnapshot` remain the coordinator payload unchanged?**
   - What we know: Existing tests already pin its current warning/launch behavior.
   - What's unclear: Whether the backend interface feels awkward if it still receives the full snapshot.
   - Recommendation: Default to keeping it unchanged in Phase 50; split only if the extraction becomes materially cleaner.

2. **Does the coordinator still need raw placeholder HWND access after backend extraction?**
   - What we know: Today it asks for `IntPtr` and readiness separately.
   - What's unclear: Whether that can collapse to backend readiness only without destabilizing the refactor.
   - Recommendation: Minimize coordinator signature churn unless it clearly reduces coupling with no behavior risk.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Windows host OS | Real child-HWND behavior proof | ✓ | Windows_NT | — |
| .NET SDK | Build/test/control-host verification | ✓ | 10.0.300 | — |
| Copied runtime bundle source | Runtime lookup preservation | ✓ | `runtime/win-x64/mdCAD.exe` present | — |

**Missing dependencies with no fallback:**
- None

**Missing dependencies with fallback:**
- None

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | xUnit `2.5.3` + `Microsoft.NET.Test.Sdk` `17.8.0` |
| Config file | `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj` (no separate runner config) |
| Quick run command | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests"` |
| Full suite command | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WPRS-01 | Preserve `mdcad-runtime\\mdCAD.exe` lookup plus `--embedded --parent-hwnd` launch path | unit + manual Windows harness | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release` plus manual `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` / harness run | ❌ Wave 0 for backend-specific launch proof |
| WPRS-02 | Preserve attach/relaunch/status behavior for sealed and diagnostic modes | unit + manual Windows harness | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests"` plus manual host lifecycle check | ⚠️ Partial — coordinator tests exist; harness regression remains manual |

### Sampling Rate
- **Per task commit:** `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~MdCadLaunchSnapshotTests"`
- **Per wave merge:** `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release` and `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Phase gate:** Windows diagnostic harness manual launch/attach/stop/relaunch pass before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` — verify preserved runtime path, working directory, and required launch args without changing public API
- [ ] `samples/avalonia-mdcad-control.tests/MdCadRuntimeResolverTests.cs` — pin `AppContext.BaseDirectory\\mdcad-runtime\\mdCAD.exe` contract explicitly
- [ ] Manual regression artifact/checklist for `samples/avalonia-host` after extraction — attach, stop, relaunch, sealed/diagnostic equivalence

## Sequencing Recommendation

1. **Create the internal contract first**
   - Add `IMdCadEmbedBackend`
   - Add `Host/Windows/` folder
   - Extract `Win32NativeMethods`

2. **Move native surface ownership next**
   - Move `EmbedNativeControlHost` and placeholder-handle lifecycle behind the backend
   - Keep shell mounting backend-provided surface into `EmbedSurfaceContainer`

3. **Move runtime + launch logic as one unit**
   - Move `MdCadRuntimeResolver` use, `ProcessStartInfo` construction, stdout/stderr capture, attach timer logic, and resize sync together
   - Do not partially split launch from attach

4. **Thin the shell without changing behavior**
   - Keep warning/status/presentation code in `MdCadEmbeddedControl`
   - Rewire `MdCadSessionCoordinator` to call backend operations

5. **Close with Windows regression proof**
   - Re-run control tests
   - Build and manually exercise `samples/avalonia-host`
   - Verify no change to sealed/diagnostic behavior text or lifecycle

## Deferred to Phases 51-54

- **Phase 51:** Add unsupported-platform backend/placeholder behavior and deterministic no-launch semantics.
- **Phase 52:** Widen the public control TFM to plain `net10.0` and solve the compile-time compatibility wall.
- **Phase 53:** Add/expand consumer proof for the widened host path while re-closing Windows runtime regression evidence.
- **Phase 54:** Update README/quickstart/onboarding wording so compile-time host compatibility and Windows-only runtime support are explicit.

## Sources

### Primary (HIGH confidence)
- Repo source:
  - `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`
  - `samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs`
  - `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs`
  - `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs`
  - `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs`
  - `samples/avalonia-host/MainWindow.axaml.cs`
  - `samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs`
  - `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs`
- Planning context:
  - `.planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-CONTEXT.md`
  - `.planning/ROADMAP.md`
  - `.planning/REQUIREMENTS.md`
  - `.planning/PROJECT.md`
  - `.planning/STATE.md`
  - `.planning/research/{SUMMARY,STACK,ARCHITECTURE,PITFALLS,FEATURES}.md`
  - `.planning/milestones/v1.8-phases/48-reusable-avalonia-mdcad-user-control/48-CONTEXT.md`
  - `.planning/milestones/v1.8-phases/47-sample-host-workflow-proof/{47-01-SUMMARY.md,47-02-SUMMARY.md}`
- Official docs:
  - Microsoft .NET target frameworks: https://learn.microsoft.com/en-us/dotnet/standard/frameworks
  - Microsoft platform compatibility analyzer / CA1416 guidance: https://learn.microsoft.com/en-us/dotnet/standard/analyzers/platform-compat-analyzer

### Secondary (MEDIUM confidence)
- NuGet package feeds for current version lines:
  - https://api.nuget.org/v3-flatcontainer/avalonia/index.json
  - https://api.nuget.org/v3-flatcontainer/microsoft.net.test.sdk/index.json
  - https://api.nuget.org/v3-flatcontainer/xunit/index.json

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - mostly repo-pinned versions and locked phase constraints, with registry checks for current lines
- Architecture: HIGH - derived from current code anchors plus phase context and milestone research
- Pitfalls: HIGH - directly tied to existing launch/attach/relaunch implementation and prior harness summaries

**Research date:** 2026-05-18
**Valid until:** 2026-06-17
