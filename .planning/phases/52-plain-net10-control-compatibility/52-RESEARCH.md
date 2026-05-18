# Phase 52: Plain net10 Control Compatibility - Research

**Researched:** 2026-05-18  
**Domain:** .NET TFM compatibility for Avalonia control libraries with internal Windows-only runtime seams  
**Confidence:** HIGH

## User Constraints

### Locked / explicit constraints from current phase brief
- Do **NOT** weaken the Windows-only embedded-runtime truth. The embedded viewer runtime remains Windows-only.
- Do **NOT** expand into Phase 52.1 runtime-refresh automation, Phase 53 consumer proof/regression closure, or Phase 54 docs truthfulness.
- Preserve the Windows diagnostic harness/runtime bundle behavior already locked in Phases 48-51.
- Public API shape should stay as stable as possible.
- Phase 51 just introduced internal backend selection via `MdCadEmbedBackendFactory`, unsupported backend behavior, blocked coordinator behavior, and truthful unsupported shell messaging.
- Plain `net10.0` hosts currently fail project restore with `NU1201` or equivalent compatibility failure against the reusable control.
- Phase requirements in scope: `HOSTC-01`, `HOSTC-02`.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| HOSTC-01 | Developer can reference `MdCad.Avalonia.Control` from a plain `net10.0` Avalonia host without `NU1201` or equivalent restore/build compatibility failures. | Retarget the control project itself to plain `net10.0`; current failure is caused by the control’s `net10.0-windows10.0.19041.0` TFM wall, not by source code or Avalonia packages. |
| HOSTC-02 | Developer can instantiate `MdCadEmbeddedControl` from shared XAML/code in a plain `net10.0` host without platform-specific type-load or startup crashes. | Keep Windows-only behavior internal behind `MdCadEmbedBackendFactory`; current control source already compiles as plain `net10.0` and selects `UnsupportedMdCadEmbedBackend` off Windows. Use existing minimal host XAML as the phase smoke surface. |

## Project Constraints (from copilot-instructions.md)

No `copilot-instructions.md` file was present at repo root during research.

## Summary

The smallest safe Phase 52 implementation is to change the reusable control project from `net10.0-windows10.0.19041.0` to plain `net10.0`, while leaving the Windows runtime implementation internal and runtime-gated exactly as Phase 51 established. The key evidence is empirical: the control project source builds successfully as plain `net10.0` when forced via MSBuild properties, including Avalonia XAML compilation and runtime content output, but a plain `net10.0` host still fails because the referenced control project remains Windows-targeted.

Nothing in the current public API requires a Windows TFM. `MdCadEmbeddedControl` exposes Avalonia types plus project-owned enums; all Win32 and `DllImport` usage is isolated under `samples/avalonia-mdcad-control/Host/Windows/` and reached through `MdCadEmbedBackendFactory`. On unsupported platforms, the control already routes to `UnsupportedMdCadEmbedBackend`, which preserves the Windows-only runtime truth without type-load crashes.

**Primary recommendation:** Retarget `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` to plain `net10.0`, keep the diagnostic harness Windows-targeted, and use `samples/avalonia-host-minimal` as the smallest Phase 52 plain-net10 proof host.

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| .NET SDK / TFM | SDK 10.0.300 installed; target `net10.0` | Host-facing contract TFM | Plain `net10.0` removes the project-reference compatibility wall while remaining referenceable from Windows-specific hosts. |
| Avalonia | 11.3.15 | Core UI/control/XAML stack | Already resolved in-repo; control source and XAML compile successfully against it. |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Avalonia.Desktop | 11.3.15 | Desktop runtime assets for host apps | Keep for current host samples/harnesses; no Phase 52 evidence requires removing it from the control project. |
| Avalonia.Themes.Fluent | 11.3.15 | Theme for sample/proof hosts | Needed in hosts, not part of the compatibility fix itself. |
| Microsoft.NET.Test.Sdk | 17.8.0 | Test execution | Use for existing xUnit control tests. |
| xUnit | 2.5.3 | Unit tests | Existing validation lane for control behavior. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Single target `net10.0` control | Multi-target `net10.0;net10.0-windows10.0.19041.0` | Multi-targeting would also solve reference compatibility, but adds duplicate build/output complexity for little gain because the current source already compiles as plain `net10.0`. |
| Retargeting the control project | Split host-facing and Windows-implementation assemblies/packages | Overkill for Phase 52; package redesign is explicitly out of scope and already deferred by `PACK-01`. |
| Existing minimal host as proof | New proof host | Unnecessary churn; `samples/avalonia-host-minimal` already instantiates `MdCadEmbeddedControl` from shared XAML/code. |

**Installation:**
```bash
dotnet restore samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj
dotnet restore samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj
```

**Version verification:** Resolved locally via `dotnet list package`:
- `Avalonia` → `11.3.15`
- `Avalonia.Desktop` → `11.3.15`
- `Avalonia.Themes.Fluent` → `11.3.15`

## Architecture Patterns

### Recommended Project Structure
```text
samples/
├── avalonia-mdcad-control/         # public reusable control library (retarget to plain net10)
│   ├── Host/                       # shared backend abstraction/coordinator/runtime-neutral shell logic
│   └── Host/Windows/               # internal Win32/backend implementation stays here
├── avalonia-host-minimal/          # smallest plain-net10 proof host
├── avalonia-host/                  # Windows diagnostic harness stays Windows-targeted
└── avalonia-mdcad-control.tests/   # existing unit/contract tests
```

### Pattern 1: Plain host-facing assembly, runtime-gated internal backend
**What:** Keep the public assembly target plain `net10.0`, but select the actual backend at runtime through the existing internal factory.  
**When to use:** Exactly this phase.  
**Example:**
```csharp
internal static IMdCadEmbedBackend Create(..., Func<bool>? isWindows = null)
{
    Func<bool> platformProbe = isWindows ?? OperatingSystem.IsWindows;
    if (!platformProbe())
    {
        return new UnsupportedMdCadEmbedBackend();
    }

    return new WindowsMdCadEmbedBackend(...);
}
```
**Source:** `samples/avalonia-mdcad-control/Host/MdCadEmbedBackendFactory.cs`

### Pattern 2: Keep Win32 and `DllImport` code internal
**What:** All Win32-specific code remains under `Host/Windows/` and is not part of the public API surface.  
**When to use:** Preserve Phase 48-51 behavior while widening the compile-time contract.  
**Example:**
```csharp
internal static class Win32NativeMethods
{
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr CreateWindowExW(...);
}
```
**Source:** `samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs`

### Pattern 3: Reuse existing minimal XAML host as the proof surface
**What:** Use `samples/avalonia-host-minimal` for Phase 52 proof instead of inventing a new sample.  
**When to use:** To prove shared XAML/code instantiation with minimal scope creep.  
**Example:**
```xml
<mdcad:MdCadEmbeddedControl PresentationMode="Sealed"
                            AutoStart="True"
                            JsonlPath="{Binding JsonlPath}" />
```
**Source:** `samples/avalonia-host-minimal/MainWindow.axaml`

### Anti-Patterns to Avoid
- **Retargeting only the host samples:** This does not remove the compatibility wall; the control project TFM is the blocker.
- **Adding new public platform-specific API:** Phase 51 already created the needed internal seam; do not leak Windows types into the public contract.
- **Multi-targeting by default:** Adds complexity without evidence of need.
- **Moving Windows behavior into docs/runtime automation work:** Phase 52 is not Phase 52.1, 53, or 54.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| TFM compatibility fix | Custom facade assembly or reflection-only wrapper | Plain `net10.0` control TFM | The problem is the control csproj target, not a missing runtime abstraction. |
| Unsupported-platform behavior | New placeholder/runtime-detection subsystem | Existing `MdCadEmbedBackendFactory` + `UnsupportedMdCadEmbedBackend` | Phase 51 already solved this cleanly and truthfully. |
| Proof host | Brand-new consumer sample | Existing `samples/avalonia-host-minimal` | It already proves shared XAML usage with minimal extra churn. |
| Windows runtime preservation | Conditional public API forks | Existing internal `Host/Windows` backend files | Preserves runtime truth and avoids public API drift. |

**Key insight:** The hard part was already done in Phase 51. Phase 52 is mostly a TFM correction, not a new architecture project.

## Common Pitfalls

### Pitfall 1: Fixing the wrong project
**What goes wrong:** A host is retargeted to plain `net10.0`, but the control remains `net10.0-windows10.0.19041.0`, so compatibility failures remain.  
**Why it happens:** The host is the visible failing consumer, but the incompatible project is the referenced control.  
**How to avoid:** Change `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` first.  
**Warning signs:** Build errors like:
- project reference incompatible with `.NETCoreApp,Version=v10.0`
- `NU1201` or equivalent compatibility failures

### Pitfall 2: Over-correcting into multi-target or package redesign
**What goes wrong:** Phase 52 becomes a packaging refactor.  
**Why it happens:** Multi-targeting feels safer than single-target retargeting.  
**How to avoid:** Prefer plain `net10.0` unless a concrete compile/runtime blocker appears after the actual csproj edit.  
**Warning signs:** New conditional compilation, duplicate content logic, or plans to split assemblies.

### Pitfall 3: Accidentally implying cross-platform embedded runtime support
**What goes wrong:** The widened compile-time contract is mistaken for cross-platform runtime support.  
**Why it happens:** The control becomes referenceable from plain `net10.0` hosts.  
**How to avoid:** Preserve the existing unsupported backend, blocked start behavior, and warning text unchanged.  
**Warning signs:** Any code/docs change that removes or softens `MdCadUnsupportedRuntime.UnsupportedRuntimeMessage`.

### Pitfall 4: Touching Windows runtime packaging in this phase
**What goes wrong:** Runtime bundle refresh/copy automation gets mixed into the compatibility fix.  
**Why it happens:** Both areas live near the control project.  
**How to avoid:** Leave `runtime/win-x64` behavior alone; Phase 52.1 owns automation.  
**Warning signs:** New post-build helpers or copy-refresh logic.

## Code Examples

Verified repo patterns:

### 1. Minimal TFM change on the control project
```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <Nullable>enable</Nullable>
    <ImplicitUsings>enable</ImplicitUsings>
    <AssemblyName>MdCad.Avalonia.Control</AssemblyName>
    <RootNamespace>MdCad.Avalonia.Control</RootNamespace>
  </PropertyGroup>

  <ItemGroup>
    <PackageReference Include="Avalonia" Version="11.3.*" />
    <PackageReference Include="Avalonia.Desktop" Version="11.3.*" />
  </ItemGroup>

  <ItemGroup>
    <Content Include="runtime\win-x64\**\*">
      <Link>mdcad-runtime\%(RecursiveDir)%(Filename)%(Extension)</Link>
      <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
      <CopyToPublishDirectory>PreserveNewest</CopyToPublishDirectory>
    </Content>
  </ItemGroup>
</Project>
```
**Source basis:** current `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` with only the `TargetFramework` changed.

### 2. Existing proof-host XAML should remain valid
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

### 3. Unsupported backend remains the non-Windows safety contract
```csharp
internal sealed class UnsupportedMdCadEmbedBackend : IMdCadEmbedBackend
{
    public bool CanStartSession => false;
    public string? StartBlockedReason => MdCadUnsupportedRuntime.UnsupportedRuntimeMessage;

    public Task StartSessionAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        throw new PlatformNotSupportedException(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage);
    }
}
```
**Source:** `samples/avalonia-mdcad-control/Host/UnsupportedMdCadEmbedBackend.cs`

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Reusable control targeted `net10.0-windows10.0.19041.0` | Reusable control should target plain `net10.0` while selecting Windows backend internally | Phase 52 recommendation, enabled by Phase 51 seam | Plain hosts can reference the control; Windows runtime truth stays internal and unchanged. |
| Windows-only behavior implicit in the control | Explicit supported/unsupported backend factory | Phase 51 | Safe plain-TFM retargeting is now possible without runtime ambiguity. |

**Deprecated/outdated:**
- **Windows-only TFM for the reusable control library:** outdated for v1.9 goals; it is the direct cause of host compatibility failures.
- **Using Quick Start’s current Windows-only TFM as implementation truth:** docs are now behind the intended milestone state and should not drive Phase 52 design decisions.

## Open Questions

1. **Should Phase 52 retarget `samples/avalonia-host-minimal` now, or leave all proof-host work to Phase 53?**
   - What we know: The host already instantiates `MdCadEmbeddedControl` from shared XAML/code and is the smallest proof surface.
   - What's unclear: Whether the team wants Phase 52 to commit that proof-host TFM change now or only use it as a local validation lane.
   - Recommendation: Retarget `samples/avalonia-host-minimal` in Phase 52, but keep validation minimal. Leave broader proof/regression closure to Phase 53.

2. **Should `Avalonia.Desktop` be removed from the control library?**
   - What we know: The control source compiles as plain `net10.0` without code changes. No evidence shows the package reference itself blocks Phase 52.
   - What's unclear: Whether the library truly needs `Avalonia.Desktop` at compile time.
   - Recommendation: Do not optimize this in Phase 52. Keep package changes minimal unless the actual retarget exposes a concrete issue.

3. **Will the proof host need conditional `ApplicationManifest` cleanup for future non-Windows CI?**
   - What we know: Current minimal host has `OutputType=WinExe` and `ApplicationManifest`.
   - What's unclear: Whether future non-Windows build agents will care.
   - Recommendation: Treat as a follow-up only if the actual plain-net10 proof host build fails on non-Windows infrastructure. Do not preemptively expand scope.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| .NET SDK | Build/restore/test for Phase 52 | ✓ | 10.0.300 (also 9.0.306 installed) | — |
| NuGet restore | Avalonia package resolution | ✓ | local restore succeeded | Use existing restored packages if offline |
| Windows runtime bundle under `runtime/win-x64` | Windows diagnostic build/runtime preservation | ✓ | repo content present | — |

**Missing dependencies with no fallback:**
- None found for Phase 52 research/validation.

**Missing dependencies with fallback:**
- None found.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | xUnit 2.5.3 + Microsoft.NET.Test.Sdk 17.8.0 |
| Config file | none |
| Quick run command | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj --filter FullyQualifiedName~MdCadEmbeddedControlTests` |
| Full suite command | `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj` |

### Phase Requirements -> Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| HOSTC-01 | Plain `net10.0` host can reference/build against control | build smoke | `dotnet build samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -c Release` | ✅ |
| HOSTC-02 | Control instantiates from shared XAML/code without startup crash | build smoke + manual startup smoke | `dotnet build samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -c Release` | ✅ |

### Additional regression guard
- Windows diagnostic harness still builds:
  - `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- Existing control behavior still passes:
  - `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release`

### Sampling Rate
- **Per task commit:** `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj --filter FullyQualifiedName~MdCadEmbeddedControlTests`
- **Per wave merge:**  
  1. `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release`  
  2. `dotnet build samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -c Release`  
  3. `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Phase gate:** All above green, plus one manual startup smoke of the plain-net10 minimal host on Windows.

### Wave 0 Gaps
- [ ] No existing automated host-launch smoke covers `HOSTC-02` startup/no-crash behavior; Phase 52 should use a minimal manual startup check instead of inventing a new UI harness.
- [ ] If `samples/avalonia-host-minimal` is retargeted in this phase, its build command must be added to the phase verification checklist.

## Sources

### Primary (HIGH confidence)
- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` — current blocker is the Windows-only TFM.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` — public control shell has no public Windows-specific API requirement.
- `samples/avalonia-mdcad-control/Host/MdCadEmbedBackendFactory.cs` — runtime-gated backend selection already exists.
- `samples/avalonia-mdcad-control/Host/UnsupportedMdCadEmbedBackend.cs` — unsupported-platform contract already preserves Windows-only runtime truth.
- `samples/avalonia-mdcad-control/Host/Windows/{EmbedNativeControlHost.cs,Win32NativeMethods.cs,WindowsMdCadEmbedBackend.cs}` — all Windows-specific code is internal.
- `samples/avalonia-host-minimal/MainWindow.axaml` — existing shared-XAML proof surface.
- `samples/avalonia-host/AvaloniaHost.csproj` — Windows diagnostic harness remains a separate Windows-targeted consumer.
- Empirical build: `dotnet build samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj -p:TargetFramework=net10.0 -p:TargetFrameworks=net10.0 -v:minimal` — succeeded, including Avalonia XAML compile.
- Empirical build: `dotnet build samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj -p:TargetFramework=net10.0 -p:TargetFrameworks=net10.0 -v:minimal` — failed because referenced control still targets `net10.0-windows10.0.19041.0`.
- `dotnet list package` on control/hosts — resolved Avalonia packages to `11.3.15`.
- `samples/avalonia-mdcad-control/bin/Debug/net10.0/` — plain-net10 control build still emitted `mdcad-runtime/` output.

### Secondary (MEDIUM confidence)
- `samples/avalonia-mdcad-control/obj/project.assets.json` — Avalonia packages restore for `net10.0` using `net8.0` assets, indicating package compatibility with the target.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — verified from local restore/build/package resolution.
- Architecture: HIGH — derived directly from current Phase 48-51 implementation.
- Pitfalls: MEDIUM — strongly supported by repo evidence and build experiments, but future CI/non-Windows proof-host details may expose additional host-project issues.

**Research date:** 2026-05-18  
**Valid until:** 2026-06-17
