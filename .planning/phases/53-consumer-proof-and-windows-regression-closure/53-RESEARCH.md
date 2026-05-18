# Phase 53: Consumer Proof and Windows Regression Closure - Research

**Researched:** 2026-05-18  
**Domain:** Consumer-proof validation for plain `net10.0` hosts plus Windows runtime regression closure  
**Confidence:** HIGH

## User Constraints

### No CONTEXT.md was present; explicit constraints from the phase brief
- Keep scope limited to **Phase 53**.
- Do **not** drift into **Phase 54** docs/onboarding work.
- Do **not** reopen **Phase 52** plain-`net10.0` compatibility work except to consume its proof surface.
- Do **not** reopen **Phase 52.1** runtime-refresh automation work except to consume its maintained runtime bundle/output contract.
- Plain `net10.0` compatibility and real Windows runtime proof are **distinct obligations** and must stay separate in planning and validation.
- Windows runtime embedding remains **Windows-only**; plain `net10.0` compatibility must **not** imply cross-platform embedding/runtime support.
- Reuse current samples, tests, validation artifacts, and harnesses where they already cover the requirement.
- Requirements in scope: `WPRS-03`, `PROOF-01`.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WPRS-03 | Windows consumer proof hosts still verify copied-runtime presence plus attach/stop/relaunch lifecycle without regressions. | Reuse `samples/avalonia-host` as the authoritative Windows proof surface, pair automated preflight coverage (`RuntimeRefreshIntegrationTests`, `MdCadSessionCoordinatorTests`, `WindowsMdCadEmbedBackendTests`, `MdCadRuntimeResolverTests`) with a fresh Phase 53 manual lifecycle checklist/run. |
| PROOF-01 | Repository includes a plain `net10.0` Avalonia consumer proof that references the control directly and builds successfully. | Use `samples/avalonia-host-minimal` as the compile/build proof surface; it already targets plain `net10.0`, references the control directly, and built successfully during research. |

## Project Constraints (from copilot-instructions.md)

No `copilot-instructions.md` file was present at repo root during research.

## Summary

Phase 53 does **not** need new proof surfaces. The repo already has the two correct surfaces this phase needs: `samples/avalonia-host-minimal` for the plain `net10.0` consumer compile/build obligation, and `samples/avalonia-host` for the real Windows runtime obligation. The minimal host is intentionally small and already targets `net10.0`; the diagnostic host is still the only truthful in-repo surface that exposes copied-runtime presence and lets a human verify attach, stop, and relaunch against the real embedded child-HWND path.

Most of the technical work is already done by earlier phases. Phase 52 proved the plain-host build path, Phase 50 proved the authoritative Windows lifecycle path, and Phase 52.1 proved the maintained runtime-bundle/output chain with hash/path checks. The remaining Phase 53 work is to recombine those finished surfaces into one phase-local proof strategy without blurring the boundary between compile-time compatibility and Windows-only runtime behavior.

**Primary recommendation:** Plan Phase 53 as a proof-and-validation closeout phase: build `samples/avalonia-host-minimal` for `PROOF-01`, then re-close Windows proof by combining existing automated runtime/lifecycle preflight with a fresh manual run of `samples/avalonia-host` for copied-runtime presence plus attach/stop/relaunch.

## Key Findings

1. **The minimal host is already the right `PROOF-01` surface.**  
   `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` targets plain `net10.0`, directly references `..\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj`, and built successfully during research.

2. **The diagnostic host is already the right `WPRS-03` surface.**  
   `samples/avalonia-host/AvaloniaHost.csproj` remains Windows-targeted and its UI already surfaces:
   - copied-runtime presence via `runtime: copied bundle present -> ...`
   - host action notes via `StartAsync requested` / `StopAsync requested`
   - real start/stop buttons and the embedded control's diagnostic status lines.

3. **Automated proof already covers most of the Windows preflight, but not the real lifecycle UAT.**  
   Existing tests already cover runtime resolution, runtime artifact presence/hash equality, launch argument generation, relaunch-safe placeholder usage, and coordinator stop/restart behavior. What remains manual is the actual host run proving attach, stop, and relaunch in the real native child-HWND path.

4. **The current phase gap is mostly artifact/validation shaping, not a new architecture gap.**  
   Research did not uncover a missing host or missing test harness that justifies new proof infrastructure. The likely Phase 53 deliverables are validation-ledger rows, a phase-local manual checklist/proof artifact, and only minimal code/test edits if a concrete gap appears while wiring those proofs together.

5. **There are two important stale-artifact traps.**  
   - `samples/avalonia-mdcad-control/QUICKSTART.md` still says hosts should target `net10.0-windows10.0.19041.0`; that is a Phase 54 docs truthfulness problem, not a Phase 53 code/proof blocker.  
   - The recorded Phase 50 manual checklist still says **Launch Session / Close Session**, while the current host buttons are labeled **StartAsync / StopAsync**. Phase 53 should not blindly reuse those words without updating the phase-local proof instructions.

## Standard Stack

### Core

| Library / Tool | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| .NET SDK | 10.0.300 | Build/test all proof surfaces | Installed locally and used successfully during research. |
| Avalonia | 11.3.15 | Shared UI/control stack | Used by both proof hosts and the reusable control. |
| Avalonia.Desktop | 11.3.15 | Desktop host runtime | Required by both in-repo host samples. |
| Microsoft.NET.Test.Sdk | 17.8.0 | Existing automated validation runner | Already owns the control/runtime proof lane. |
| xUnit | 2.5.3 | Existing test framework | Already covers coordinator/backend/runtime assertions needed for preflight. |

### Supporting

| Library / Tool | Version | Purpose | When to Use |
|----------------|---------|---------|-------------|
| Avalonia.Themes.Fluent | 11.3.15 | Sample-host theme | Needed by host proof apps, not by proof strategy itself. |
| coverlet.collector | 6.0.0 | Test collector dependency | Leave as-is in the existing test project. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `samples/avalonia-host-minimal` for compile/build proof | New throwaway proof host | Unnecessary churn; the minimal host already exists for exactly this purpose. |
| `samples/avalonia-host` for Windows lifecycle proof | New diagnostic verifier app or UI automation harness | Too much scope for a closeout phase; the current host is already the authoritative runtime surface. |
| Existing unit/integration preflight + manual UAT | Full automated native UI harness | Not justified here; Phase 50 explicitly kept real child-HWND lifecycle proof manual. |

**Version verification used during research:**
- `dotnet --version` → `10.0.300`
- `dotnet list samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj package`
- `dotnet list samples/avalonia-host/AvaloniaHost.csproj package`
- `dotnet list samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj package`

## Recommended Phase Breakdown

### Plan 1: Lock the plain `net10.0` consumer proof surface
- Treat `samples/avalonia-host-minimal` as the sole `PROOF-01` compile/build surface.
- Record/verify that the host still builds directly against the control project with no Windows-TFM compatibility wall.
- Keep this plan compile/build-only; do not expand into runtime attach semantics.

### Plan 2: Assemble the automated Windows regression preflight
- Reuse existing tests instead of inventing new proof infrastructure:
  - `RuntimeRefreshIntegrationTests`
  - `MdCadRuntimeResolverTests`
  - `WindowsMdCadEmbedBackendTests`
  - `MdCadSessionCoordinatorTests`
- Rebuild `samples/avalonia-host` in Release and treat it as the consumer-output source for copied-runtime verification.
- Only add code/tests if a concrete proof gap appears while composing this lane.

### Plan 3: Re-run and record the authoritative Windows host lifecycle proof
- Run `samples/avalonia-host` as the real proof surface.
- Verify copied runtime presence, attach, stop, and relaunch against the real embedded session.
- Record the result in a **Phase 53** artifact instead of mutating historical Phase 50 proof in place.

**Minimal sufficient split:** 3 plans. Anything larger is likely proof-surface churn; anything smaller risks mixing compile proof and runtime proof into one blurry validation story.

## Architecture Patterns

### Recommended Project Structure

```text
samples/
├── avalonia-host-minimal/          # plain net10 compile/build proof host
├── avalonia-host/                  # Windows-only runtime proof harness
├── avalonia-mdcad-control/         # reusable control + copied runtime content owner
└── avalonia-mdcad-control.tests/   # automated preflight coverage
```

### Pattern 1: Separate compile/build proof from runtime proof
**What:** Use different proof surfaces for different obligations.  
**When to use:** Entire Phase 53.  
**Example:**
```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>WinExe</OutputType>
    <TargetFramework>net10.0</TargetFramework>
  </PropertyGroup>

  <ItemGroup>
    <ProjectReference Include="..\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj" />
  </ItemGroup>
</Project>
```
**Source:** `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj`

### Pattern 2: Keep Windows runtime proof on the diagnostic host
**What:** Use the existing harness status lines and manual controls instead of a new runtime verifier.  
**When to use:** `WPRS-03`.  
**Example:**
```csharp
private string BuildRuntimeStatus()
{
    string executablePath = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "mdcad-runtime", RuntimeExecutableName));
    return File.Exists(executablePath)
        ? $"runtime: copied bundle present -> {executablePath}"
        : $"runtime: copied bundle missing -> {executablePath}";
}
```
**Source:** `samples/avalonia-host/MainWindow.axaml.cs`

### Pattern 3: Use automated preflight for artifact/lifecycle invariants, manual proof for real attach
**What:** Let tests prove invariant behavior; let the host run prove the real native session.  
**When to use:** Windows closeout.  
**Example:**
```csharp
Assert.Equal(sourceHash, committedHash);
Assert.Equal(sourceHash, hostHash);
```
**Source:** `samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs`

### Anti-Patterns to Avoid
- **One proof surface for both obligations:** It blurs `PROOF-01` and `WPRS-03`.
- **New proof harnesses:** Existing hosts already map to the requirements.
- **Re-proving Phase 52.1 helper internals:** Phase 53 should consume the maintained bundle/output contract, not redesign or re-scope automation.
- **Treating plain `net10.0` build success as runtime support:** compile compatibility is not embedding support.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Plain host proof | New consumer sample | `samples/avalonia-host-minimal` | Already the smallest direct ProjectReference proof host. |
| Windows lifecycle proof | New runtime harness | `samples/avalonia-host` | Already exposes runtime presence and start/stop actions. |
| Lifecycle regression automation | New backend simulation layer | Existing coordinator/backend/runtime tests | Those tests already cover the code-owned invariants. |
| Runtime copy proof | Custom filesystem checker | `RuntimeRefreshIntegrationTests` + host build output | Already validates bundle/output presence and resolver contract. |

**Key insight:** Phase 53 is a proof composition problem, not a missing-infrastructure problem.

## Validation Implications

- `PROOF-01` should stay a **build smoke**: `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release`
- `WPRS-03` should stay split:
  - **Automated preflight:** host build + targeted test lane
  - **Manual runtime proof:** actual `samples/avalonia-host` run for attach/stop/relaunch
- The Windows proof lane should **not** require `-p:MdCadRefreshWindowsRuntime=true` by default. That property belongs to Phase 52.1 automation proof. Use it only as optional prep when the committed runtime bundle was intentionally refreshed in the same session.
- The proof artifact for Phase 53 should explicitly say which command proves **compile-time consumer compatibility** and which command/checklist proves **Windows runtime behavior**.

## Common Pitfalls

### Pitfall 1: Collapsing compile proof and runtime proof into one command
**What goes wrong:** The phase appears covered, but no artifact clearly proves which requirement was satisfied.  
**Why it happens:** Both hosts reference the same control project.  
**How to avoid:** Give `PROOF-01` and `WPRS-03` separate validation rows and separate proof commands.  
**Warning signs:** Validation text that says only “build both hosts” with no requirement split.

### Pitfall 2: Reopening Phase 52.1 instead of consuming it
**What goes wrong:** The phase starts changing refresh-helper behavior or native-build orchestration.  
**Why it happens:** The runtime-copy proof is adjacent to the automation work.  
**How to avoid:** Treat committed/runtime output as the input contract; only use existing refresh automation as optional prep.  
**Warning signs:** New helper arguments, new copy policy, or CMake-centric changes in Phase 53 plans.

### Pitfall 3: Assuming automated tests already prove real attach/stop/relaunch
**What goes wrong:** The phase skips the actual host lifecycle run.  
**Why it happens:** Coordinator/backend tests already cover many restart invariants.  
**How to avoid:** Keep the real `samples/avalonia-host` run as the authoritative manual proof.  
**Warning signs:** Plans that stop at `dotnet test` without any host-run artifact.

### Pitfall 4: Letting stale docs/checklists widen scope
**What goes wrong:** Phase 53 drifts into docs cleanup or historical artifact edits.  
**Why it happens:** `QUICKSTART.md` and the Phase 50 checklist contain stale wording relative to current scope/UI labels.  
**How to avoid:** Create a Phase 53 proof artifact; defer consumer-facing doc truthfulness to Phase 54.  
**Warning signs:** Tasks editing QUICKSTART/README or rewriting Phase 50 history instead of creating Phase 53 records.

## Code Examples

### Plain `net10.0` proof host surface
```xml
<Window xmlns="https://github.com/avaloniaui"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        xmlns:mdcad="clr-namespace:MdCad.Avalonia.Control;assembly=MdCad.Avalonia.Control"
        x:Class="AvaloniaHostMinimal.MainWindow">
  <Grid Margin="12">
    <mdcad:MdCadEmbeddedControl PresentationMode="Sealed"
                                AutoStart="True"
                                JsonlPath="{Binding JsonlPath}" />
  </Grid>
</Window>
```
**Source:** `samples/avalonia-host-minimal/MainWindow.axaml`

### Windows proof host lifecycle actions
```csharp
private async void OnStartViewerClick(object? sender, RoutedEventArgs e)
{
    await _embeddedControl.StartAsync();
    _lastActionNote = "StartAsync requested";
}

private async void OnStopViewerClick(object? sender, RoutedEventArgs e)
{
    await _embeddedControl.StopAsync();
    _lastActionNote = "StopAsync requested";
}
```
**Source:** `samples/avalonia-host/MainWindow.axaml.cs`

### Relaunch-safe coordinator behavior already covered automatically
```csharp
await coordinator.StartAsync();
await coordinator.StopAsync();

harness.Snapshot = MdCadLaunchSnapshot.Create(
    Path.Combine(Path.GetTempPath(), "phase50-second.jsonl"),
    startupLiveRefreshEnabled: true);

await coordinator.StartAsync();

Assert.Equal(2, harness.StartedSnapshots.Count);
Assert.Equal(harness.Snapshot, harness.StartedSnapshots[^1]);
```
**Source:** `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs`

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Phase 50 used the diagnostic host as the sole proof surface | Phase 53 should keep the diagnostic host for Windows runtime proof but add the minimal host as the distinct plain-`net10.0` consumer proof | Phase 52 introduced the minimal host; Phase 53 must formalize the split | Prevents compile/runtime proof from getting conflated |
| Runtime refresh proof focused on hash/path/output contract | Phase 53 should consume that contract and re-focus on consumer-facing runtime lifecycle | Phase 52.1 | Avoids reopening automation scope |
| Historical Phase 50 checklist captured lifecycle proof | Phase 53 should create a new phase-local proof record instead of overwriting historical proof | Phase 53 recommendation | Preserves audit trail and lets wording match current UI |

**Deprecated / outdated for this phase:**
- Treating `QUICKSTART.md` as the source of truth for Phase 53 proof scope
- Treating the Phase 50 manual checklist text as drop-in current instructions without checking current host labels

## Open Questions

1. **Should Phase 53 create a new manual checklist or reuse the Phase 50 checklist?**
   - What we know: Phase 50 already proved the right behavior, but its artifact is historical and has stale button wording.
   - What's unclear: Whether planner should copy/update that structure or add a shorter phase-local proof summary.
   - Recommendation: Create a Phase 53-scoped checklist/proof artifact that references Phase 50 expectations but matches current UI labels and Phase 53 requirement split.

2. **Should Phase 53 host builds pass `MdCadRefreshWindowsRuntime=true`?**
   - What we know: Phase 52.1 already proved the helper and the committed/runtime outputs are present now.
   - What's unclear: Whether every Phase 53 proof run needs to re-exercise helper automation.
   - Recommendation: Default to normal `dotnet build`; only use the property as optional prep when a native runtime refresh was intentionally performed in the same session.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| .NET SDK | All Phase 53 build/test commands | ✓ | 10.0.300 | — |
| Windows OS | Real runtime attach/stop/relaunch proof | ✓ | `Windows_NT` | none |
| Committed runtime bundle | Windows consumer proof | ✓ | `samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe` present | rebuild/copy from prior phase only if missing |
| Diagnostic host copied runtime output | Automated copied-runtime preflight | ✓ | `samples/avalonia-host/bin/Release/net10.0-windows10.0.19041.0/mdcad-runtime/mdCAD.exe` present | rebuild host |
| Minimal host output | Plain consumer proof artifact | ✓ | `samples/avalonia-host-minimal/bin/Release/net10.0/` present | rebuild host |

**Missing dependencies with no fallback:**
- None found during research.

**Missing dependencies with fallback:**
- None found during research.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | xUnit 2.5.3 + `Microsoft.NET.Test.Sdk` 17.8.0 + `dotnet build` smoke lanes |
| Config file | none |
| Quick run command | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` |
| Full suite command | `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests" && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PROOF-01 | Plain `net10.0` consumer host builds directly against the control | build smoke | `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release` | ✅ |
| WPRS-03 | Windows consumer output still contains copied runtime and resolves through the existing runtime contract | integration + unit | `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests"` | ✅ |
| WPRS-03 | Real host attach / stop / relaunch still works against the copied runtime | manual runtime proof | `dotnet run --project .\samples\avalonia-host\AvaloniaHost.csproj -c Release` | ❌ Plan 03 phase-local checklist artifact |

### Sampling Rate

- **Per task commit:** smallest impacted command from the map above
- **Per wave merge:** `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~RuntimeRefresh|FullyQualifiedName~MdCadSessionCoordinatorTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests|FullyQualifiedName~MdCadRuntimeResolverTests" && dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release`
- **Phase gate:** full suite green plus approved Windows manual proof artifact before `/gsd-verify-work`

### Wave 0 Gaps

- Existing automated infrastructure covers the proof/build lanes Phase 53 needs.
- The phase-local validation ledger is required up front.
- The manual checklist/proof artifact belongs to **Plan 3**, not Wave 0, because it records the final human runtime proof outcome rather than enabling earlier automated work.

## Risks and Boundary Traps

- **Scope drift into Phase 54:** `QUICKSTART.md` is stale, but Phase 53 should not become a docs cleanup phase.
- **Scope drift back into Phase 52:** A green minimal-host build is enough for the compile proof obligation; do not turn Phase 53 into another compatibility-retarget phase.
- **Scope drift back into Phase 52.1:** Runtime refresh helper behavior, copy policy, and CMake orchestration are already phase-owned elsewhere.
- **Historical artifact mutation:** Rewriting Phase 50 records would blur the audit trail; create Phase 53 proof records instead.
- **False confidence from automated-only evidence:** The real attach/stop/relaunch proof still requires a human host run.

## Key Assumptions

- The committed runtime bundle and current host outputs produced in Phase 52.1 remain the baseline input contract for Phase 53.
- No new public control API is required for Phase 53; proof can be closed with current hosts/tests/artifacts.
- Real native child-HWND lifecycle proof remains manual because the repo still has no safe headless/UI automation harness for that boundary.
- The planner should prefer artifact/validation tasks first and only introduce code changes if a concrete proof gap is exposed while wiring the phase.

## Sources

### Primary (HIGH confidence)
- `.planning/ROADMAP.md` - Phase 53 goal, dependencies, and success criteria
- `.planning/STATE.md` - current milestone position and locked scope reminders
- `.planning/REQUIREMENTS.md` - `WPRS-03` and `PROOF-01`
- `.planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-MANUAL-CHECKLIST.md` - authoritative prior Windows lifecycle proof structure
- `.planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-03-SUMMARY.md` - why Phase 50 kept real lifecycle proof manual
- `.planning/phases/52-plain-net10-control-compatibility/52-03-SUMMARY.md` and `52-VALIDATION.md` - current plain-host proof lane
- `.planning/phases/52.1-automate-windows-runtime-refresh-from-build-vulkan-with-a-dotnet-managed-post-build-helper/52.1-03-SUMMARY.md` and `52.1-VALIDATION.md` - current runtime-copy/output proof lane
- `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` and `MainWindow.axaml` - plain `net10.0` proof host surface
- `samples/avalonia-host/AvaloniaHost.csproj`, `MainWindow.axaml`, and `MainWindow.axaml.cs` - Windows proof host surface
- `samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs` - copied-runtime/output contract checks
- `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` - stop/relaunch invariant checks
- `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` - launch-arg and relaunch placeholder invariants
- `samples/avalonia-mdcad-control.tests/MdCadRuntimeResolverTests.cs` and `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs` - runtime-root contract
- `samples/avalonia-mdcad-control/QUICKSTART.md` - current docs mismatch to keep out of scope
- Executed locally on 2026-05-18:
  - `dotnet --version`
  - `dotnet list ... package`
  - `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release -v minimal`
  - `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release`
  - `dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release`

### Secondary (MEDIUM confidence)
- None.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - verified from current project files and local package resolution
- Architecture: HIGH - based on current host/control/test structure and completed prior-phase artifacts
- Pitfalls: HIGH - directly evidenced by current repo history, stale artifacts, and requirement boundaries

**Research date:** 2026-05-18  
**Valid until:** 2026-06-17
