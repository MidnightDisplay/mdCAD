# Phase 48: Reusable Avalonia mdCAD user control - Research

**Researched:** 2026-05-15
**Domain:** Windows-only Avalonia reusable control packaging for the existing mdCAD child-HWND host
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Control startup and configuration contract
- **D-01:** The reusable control supports both auto-launch and explicit start.
- **D-02:** Auto-launch is the default behavior when the control is dropped into XAML and configured through properties.
- **D-03:** The owning app configures the control through bindable control properties rather than a constructor-only options object.
- **D-04:** Changing launch-affecting startup properties after mdCAD is already running triggers a control-managed relaunch with the new startup arguments.

### mdCAD runtime payload packaging
- **D-05:** Phase 48 ships a repo-owned pinned/stable mdCAD runtime bundle; the consuming app does not build mdCAD from source during its own build.
- **D-06:** The shipped payload is a full mdCAD runtime folder, not just `mdCAD.exe`.
- **D-07:** The reusable control project copies that full runtime folder into a dedicated subfolder under the consuming app's output directory.
- **D-08:** The control always resolves mdCAD from the copied stable runtime bundle; the consuming app does not provide or override the runtime path.

### Control presentation modes
- **D-09:** The control has two presentation modes: `sealed` and `diagnostic`.
- **D-10:** `sealed` is the default mode.
- **D-11:** `diagnostic` mode is explicit opt-in from the owning app.
- **D-12:** `diagnostic` mode carries over the sample host's controls plus status/failure surface and renders them above the mdCAD native host region.
- **D-13:** `sealed` mode stays a bare viewer surface instead of exposing the sample host's management chrome.

### Optional JSONL startup behavior
- **D-14:** If `JsonlPath` is intentionally unset or empty, the control launches mdCAD normally without `--jsonl` and without showing a warning.
- **D-15:** If `JsonlPath` is set but points to a missing or unreadable file, the control shows a visible warning and still launches an empty usable viewer.
- **D-16:** The control must not silently hide or swallow a bad requested JSONL path.

### the agent's Discretion
- Choose the exact Avalonia property names, types, and change-notification wiring for the reusable control, as long as they remain bindable and support the locked relaunch behavior.
- Choose the exact dedicated runtime-subfolder name and MSBuild copy mechanics, as long as the pinned full runtime folder lands deterministically under the consuming app's output.
- Choose the exact relaunch throttling/debounce behavior for rapid property changes, as long as host apps do not have to manage mdCAD restarts themselves.
- Choose the exact visual treatment of the sealed-mode warning surface, as long as a bad requested JSONL path is visible without turning the sealed control into the full diagnostic chrome.

### Deferred Ideas (OUT OF SCOPE)
- NuGet packaging/distribution is explicitly future work and out of scope for this phase.
- Rich host-to-viewer IPC, machine-readable readiness/import events, and post-launch command channels remain future host integration work.
- Multi-viewer management and broader embedded-host orchestration remain future phases if needed.
- Cross-platform reusable host controls remain out of scope; this phase stays Windows + Avalonia only.
</user_constraints>

## Project Constraints (from copilot-instructions.md)

None — `copilot-instructions.md` is not present in the repo root.

## Summary

Do **not** turn `samples/avalonia-host` directly into the only deliverable. The clean Phase 48 shape is: extract the proven HWND/process lifecycle code into a new Windows-only Avalonia **class library** that exposes a reusable `UserControl`, then keep `samples/avalonia-host` as the in-repo diagnostic harness that references that library like an external app would. That preserves the already-validated Phase 43-47 child-window/focus/orphan behavior while giving the planner a real consumer app for proof.

The runtime bundle should also stop depending on repo-root discovery. The current host finds `build-vulkan/bin/Release/mdCAD.exe` by walking up to `.planning`/`src`; that is exactly what a cross-repo consumer cannot rely on. Instead, Phase 48 should ship a curated pinned runtime folder inside the control project and copy it to a dedicated output subfolder such as `mdcad-runtime/` using standard MSBuild content metadata. Then the control resolves `mdCAD.exe` from `AppContext.BaseDirectory`, sets `WorkingDirectory` to that copied runtime folder, and launches via the existing CLI contract only.

Property-change relaunch must be serialized, coalesced, and snapshot-based. Do not restart mdCAD directly from every Avalonia property setter/change callback. Use one control-owned lifecycle coordinator that (1) classifies launch-affecting vs non-launch-affecting properties, (2) queues one desired launch snapshot, (3) tears down the current session through the already-proven graceful-wait/fallback-kill path, (4) recreates the placeholder HWND before relaunch, and (5) starts only the newest requested generation. That is the safest way to preserve the current no-IPC/process-boundary model without introducing orphan races.

**Primary recommendation:** Build a new Windows-only Avalonia `UserControl` library plus pinned `mdcad-runtime/` content copy, keep the sample app as the diagnostic consumer, and use a single serialized relaunch coordinator instead of ad hoc property-triggered restarts.

## Reusable Assets

| Asset | Where | Reuse Guidance |
|---|---|---|
| `EmbedNativeControlHost` | `samples/avalonia-host/MainWindow.axaml.cs` | Keep this as the internal `NativeControlHost` seam. It already uses `CreateNativeControlCore` / `DestroyNativeControlCore` correctly for a child HWND placeholder on Windows. |
| Placeholder recreation | `RecreateEmbedSurface()` in `MainWindow.axaml.cs` | Preserve this exactly in spirit. Phase 47 proved that relaunch must recreate the placeholder/native host surface before the next attach. |
| Graceful-close + fallback cleanup | `WaitForGracefulExitOnClose`, `BeginGracefulTeardownWait`, `EnsureAttachedProcessTeardown`, `FinalizeTeardownStatus` | This is the strongest existing orphan-prevention seam. Reuse it inside a control-owned lifecycle coordinator rather than rewriting process teardown logic. |
| Host-owned honest status text | `SetLaunchStatus`, `SetAttachStatus`, `UpdateJsonlStatusAfterLaunch`, `UpdateLiveRefreshStatusAfterLaunch` | Carry this into `diagnostic` mode. Keep the wording host-owned and switch to `viewer-managed` after attach; do not invent readiness/import IPC. |
| Output-rooted path resolution pattern | `TryResolveBundledExample()` | Reuse the `AppContext.BaseDirectory` pattern, but point it at the copied mdCAD runtime subfolder instead of repo-relative build outputs. |
| Existing CLI contract | `src/app_launch_config.h` | Preserve `--embedded`, `--parent-hwnd`, optional `--jsonl`, optional `--jsonl-live-refresh`. Do not add a second runtime control channel. |
| Embedded layout persistence rules | `src/embed_layout_state.h`, `src/imgui_storage.h` | Important packaging finding: mdCAD resolves `imgui.embedded.ini` relative to the process working directory, so the control must launch with `WorkingDirectory = <copied runtime folder>`. |

## Recommended Phase Split

1. **Library extraction and public control surface**
   - Create the reusable Windows-only Avalonia library project.
   - Move HWND/process lifecycle code behind a control API.
   - Keep the existing sample app as the first consumer of that library.
2. **Pinned runtime bundle ownership and deterministic output copy**
   - Materialize a curated repo-owned runtime folder.
   - Add MSBuild content metadata so a consuming app receives `mdcad-runtime/**` under its own output.
   - Replace repo-root executable discovery with `AppContext.BaseDirectory` resolution.
3. **Bindable startup properties and serialized relaunch coordinator**
   - Add bindable properties for `JsonlPath`, live refresh, auto-start, and presentation mode.
   - Add explicit `Start`/`Stop` APIs for hosts using `AutoStart=false`.
   - Centralize relaunch coalescing and stale-generation cancellation.
4. **Presentation-mode polish and proof harness closure**
   - Implement `sealed` vs `diagnostic`.
   - Keep warning behavior visible but minimal in sealed mode.
   - Update the sample harness/manual checklist to prove output copy, attach, relaunch, and orphan safety.

## Standard Stack

### Core
| Library / Runtime | Version | Purpose | Why Standard |
|---|---|---|---|
| Avalonia | 11.3.15 | XAML control framework and property system | Already resolved in-repo; Phase 48 should reuse the proven stack instead of turning into an Avalonia upgrade phase. |
| Avalonia.Desktop | 11.3.15 | Desktop hosting including `NativeControlHost` | Existing sample already proves the required HWND hosting seam on this stack. |
| Avalonia.Themes.Fluent | 11.3.15 | Minimal diagnostic/sample chrome styling | Keeps the sample harness consistent without adding another styling layer. |
| .NET | `net8.0-windows10.0.19041.0` | Process APIs, path resolution, Windows-only TFM | Matches the sample host and cleanly encodes the Windows-only boundary. |

### Supporting
| Library / Tool | Version | Purpose | When to Use |
|---|---|---|---|
| MSBuild SDK content pipeline | SDK 9.0.306 observed locally | Copy repo-owned runtime content into consumer output via `CopyToOutputDirectory`, `CopyToPublishDirectory`, and `TargetPath`/`Link` | Use for the pinned mdCAD bundle; do not replace it with post-build scripts. |
| Existing mdCAD CLI launch contract | current repo | Launch the external process with `--embedded`, `--parent-hwnd`, optional `--jsonl`, optional `--jsonl-live-refresh` | Use for every control launch; no new IPC or in-process bridge. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| `UserControl` with fixed internal layout | `TemplatedControl` | `TemplatedControl` is better for lookless/restylable controls, but this phase wants a fixed viewer shell with only two built-in modes. |
| MSBuild content copy into `mdcad-runtime/` | Custom post-build PowerShell/BAT copy step | Scripts are less portable, harder to compose through `ProjectReference`, and easier to break in external consumers. |
| Staying on Avalonia 11.3.15 | Upgrading to Avalonia 12.0.3 | 12.0.3 is currently available, but a major-version upgrade is unrelated risk and not required by any locked decision. |

**Installation / project references:**
```xml
<ProjectReference Include="..\..\mdCAD\samples\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj" />
```

**Version verification:** `dotnet list samples/avalonia-host/AvaloniaHost.csproj package` resolves `Avalonia`, `Avalonia.Desktop`, and `Avalonia.Themes.Fluent` to `11.3.15`. NuGet shows `11.3.15` published on 2026-05-10; latest major available is `12.0.3` (also 2026-05-10), but Phase 48 should stay on the repo-resolved 11.x line.

## Architecture Patterns

### Recommended Project Structure
```text
samples/
├── avalonia-mdcad-control/                 # new reusable Windows-only control library
│   ├── MdCad.Avalonia.Control.csproj
│   ├── MdCadEmbeddedControl.axaml
│   ├── MdCadEmbeddedControl.axaml.cs
│   ├── Host/
│   │   ├── EmbedNativeControlHost.cs       # HWND placeholder seam
│   │   ├── MdCadSessionCoordinator.cs      # serialized launch/relaunch/teardown
│   │   ├── MdCadLaunchSnapshot.cs          # launch-affecting property snapshot
│   │   └── MdCadRuntimeResolver.cs         # AppContext-based runtime path lookup
│   └── runtime/
│       └── win-x64/                        # curated pinned mdCAD runtime bundle
└── avalonia-host/                          # keep as diagnostic harness consuming the library
    ├── AvaloniaHost.csproj
    ├── MainWindow.axaml
    └── MainWindow.axaml.cs
```

### Pattern 1: Extract a reusable control, keep the sample app as the proof consumer
**What:** Create a new class library for the reusable control and keep `samples/avalonia-host` as the in-repo diagnostic app that references it.

**When to use:** Immediately. This phase needs both a reusable artifact and a real consumer/harness.

**Why:** A pure library gives external-app reuse; keeping the sample app preserves the already-proven manual/UAT surface and avoids embedding test-only chrome into the production control API.

### Pattern 2: Use `UserControl` + `StyledProperty` for bindable startup configuration
**What:** Register bindable `StyledProperty`s for startup configuration and keep launch state reconciliation in one coordinator.

**Recommended public surface:**
- `string? JsonlPath`
- `bool StartupLiveRefreshEnabled`
- `bool AutoStart = true`
- `MdCadPresentationMode PresentationMode = Sealed`
- `void Start()` / `Task StartAsync()`
- `void Stop()` / `Task StopAsync()`

**Launch-affecting properties:** `JsonlPath`, `StartupLiveRefreshEnabled`

**Non-launch-affecting properties:** `PresentationMode`, `AutoStart` (except initial-start behavior)

**Example:**
```csharp
// Source: Avalonia property guidance + current sample-host bindable needs
public static readonly StyledProperty<string?> JsonlPathProperty =
    AvaloniaProperty.Register<MdCadEmbeddedControl, string?>(nameof(JsonlPath));

public static readonly StyledProperty<bool> StartupLiveRefreshEnabledProperty =
    AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(StartupLiveRefreshEnabled));

public static readonly StyledProperty<bool> AutoStartProperty =
    AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(AutoStart), defaultValue: true);

public static readonly StyledProperty<MdCadPresentationMode> PresentationModeProperty =
    AvaloniaProperty.Register<MdCadEmbeddedControl, MdCadPresentationMode>(
        nameof(PresentationMode),
        defaultValue: MdCadPresentationMode.Sealed);
```

### Pattern 3: Resolve mdCAD from a copied runtime subfolder under `AppContext.BaseDirectory`
**What:** The control should always resolve the runtime from a deterministic subfolder under the consuming app output, not from the repo or a caller-supplied path.

**When to use:** Every launch.

**Example:**
```csharp
// Source: current sample-host AppContext pattern + locked Phase 48 runtime-bundle decisions
string runtimeRoot = Path.Combine(AppContext.BaseDirectory, "mdcad-runtime");
string exePath = Path.Combine(runtimeRoot, "mdCAD.exe");

var startInfo = new ProcessStartInfo
{
    FileName = exePath,
    WorkingDirectory = runtimeRoot,
    UseShellExecute = false,
    RedirectStandardOutput = true,
    RedirectStandardError = true,
};

startInfo.ArgumentList.Add("--embedded");
startInfo.ArgumentList.Add("--parent-hwnd");
startInfo.ArgumentList.Add($"0x{parentHwnd.ToInt64():X}");
```

### Pattern 4: Use standard MSBuild content metadata for the pinned runtime folder
**What:** Put the curated runtime bundle under the control project and mark it as content copied into a dedicated output subfolder.

**When to use:** For every file in the pinned runtime bundle.

**Example:**
```xml
<!-- Source: Microsoft MSBuild item metadata docs + SDK transitive copy targets -->
<ItemGroup>
  <Content Include="runtime\win-x64\**\*">
    <Link>mdcad-runtime\%(RecursiveDir)%(Filename)%(Extension)</Link>
    <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
    <CopyToPublishDirectory>PreserveNewest</CopyToPublishDirectory>
  </Content>
</ItemGroup>
```

### Pattern 5: Serialize relaunches through one lifecycle gate
**What:** Coalesce property changes into a single desired launch snapshot and run teardown/startup serially.

**When to use:** Any time a running control receives a launch-affecting property change.

**Example:**
```csharp
// Source: current sample-host teardown pattern, adapted for coalesced property changes
private readonly SemaphoreSlim _lifecycleGate = new(1, 1);
private long _requestedGeneration;

private void RequestReconcile()
{
    var generation = Interlocked.Increment(ref _requestedGeneration);
    Dispatcher.UIThread.Post(() => _ = ReconcileAsync(generation));
}

private async Task ReconcileAsync(long generation)
{
    await _lifecycleGate.WaitAsync();
    try
    {
        if (generation != _requestedGeneration || _isDisposed)
            return;

        var snapshot = CaptureLaunchSnapshot();
        if (_sessionRunning)
            await StopCurrentSessionAsync();   // graceful wait, fallback kill, recreate placeholder

        if (generation != _requestedGeneration || !ShouldStart(snapshot))
            return;

        await StartSessionAsync(snapshot);
    }
    finally
    {
        _lifecycleGate.Release();
    }
}
```

### Anti-Patterns to Avoid
- **Do not keep repo-root discovery:** `FindRepoRoot()` + `build-vulkan/bin/Release/mdCAD.exe` is sample-only and breaks the cross-repo local-reference goal.
- **Do not copy the whole `build-vulkan/bin/Release` folder:** it currently contains test executables in addition to `mdCAD.exe`; create a curated runtime bundle instead.
- **Do not launch from every property setter/change event:** that creates overlapping teardown/startup races and stale HWND reuse.
- **Do not reuse a dead placeholder HWND after close:** Phase 47 proved relaunch needs placeholder recreation first.
- **Do not call `SetFocus`/`SetActiveWindow` on the external child HWND from the host:** Phase 44 already established that host-side focus forcing is unreliable and should stay out.
- **Do not choose `runtime` or `runtimes` as the subfolder name:** Avalonia app output already contains a `runtimes/` folder; use a unique folder such as `mdcad-runtime/`.
- **Do not expose test-mode flags (`invalid-parent`, `destroy-after-attach`) on the reusable control API:** keep those in the harness only.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Runtime-bundle output copy | Post-build shell scripts | Standard MSBuild `Content`/`Link`/`CopyToOutputDirectory`/`CopyToPublishDirectory` metadata | The SDK already copies content from project references and preserves `TargetPath`. |
| Runtime path discovery | Repo-root walking | `AppContext.BaseDirectory` + fixed `mdcad-runtime/` subfolder | Works for cross-repo consumers and matches the sample's bundled-example pattern. |
| Live reconfiguration channel | Host-to-viewer IPC | Relaunch with the existing CLI flags | Locked scope keeps mdCAD as a separate process with no new IPC surface. |
| Focus ownership | Host-side cross-process focus forcing | Existing child-HWND first-click focus rules from Phases 44-47 | The prior phase evidence already proved the safe focus boundary. |
| Relaunch scheduling | Multiple timers/setters each launching independently | Single serialized session coordinator with generation coalescing | Prevents orphaned processes, stale attach polling, and dead-placeholder reuse. |

**Key insight:** The repo already solved the hard problems at the native boundary. Phase 48 should package and orchestrate those seams, not redesign them.

## Common Pitfalls

### Pitfall 1: Auto-start fires before bindings and the placeholder HWND are both ready
**What goes wrong:** The control launches with default/empty properties or before the `NativeControlHost` placeholder exists.

**Why it happens:** Avalonia bindings can update after construction, while the HWND placeholder is only available after the control is attached and the native host is created.

**How to avoid:** Defer startup until both conditions are true: visual attachment/native placeholder ready and a coalesced property snapshot has settled. Then launch once from the latest snapshot.

**Warning signs:** Duplicate launches during window startup; `JsonlPath` ignored on first render; launch attempted with `parent hwnd = 0`.

### Pitfall 2: Relaunch reuses stale attach state
**What goes wrong:** The new mdCAD instance never attaches, or attaches to a dead placeholder, leaving orphaned processes.

**Why it happens:** The current sample proved that teardown destroys the old native hosting seam; relaunch needs a fresh placeholder HWND.

**How to avoid:** Clear attach/process state, destroy/invalidate the old placeholder, wait for exit, then recreate the host surface before the next launch.

**Warning signs:** Attach timeout on second launch only; child process exists but no viewer is shown.

### Pitfall 3: Working directory points at the host output root instead of the copied runtime folder
**What goes wrong:** `imgui.embedded.ini` and any future relative runtime assets land beside the host app or resolve inconsistently.

**Why it happens:** `src/imgui_storage.h` resolves relative store paths from the process working directory.

**How to avoid:** Set `ProcessStartInfo.WorkingDirectory` to the copied runtime subfolder every time.

**Warning signs:** `imgui.embedded.ini` appears in the wrong folder; layout persistence changes when the host is launched from a different directory.

### Pitfall 4: Invalid `JsonlPath` handling accidentally breaks the usable-viewer guarantee
**What goes wrong:** The control either crashes launch on a bad path or silently swallows a requested path.

**Why it happens:** `app_launch_config.h` requires `--jsonl` to be an absolute path syntactically, but runtime file accessibility is handled later and non-fatally.

**How to avoid:** Normalize non-empty paths to absolute first. If canonicalization fails, warn and omit `--jsonl`. If the path is absolute but missing/unreadable, warn and still pass it so mdCAD can show its own non-fatal startup error overlay.

**Warning signs:** `--jsonl` parse failures for user-entered paths; no visible warning when a bound path is wrong.

### Pitfall 5: The pinned runtime bundle is committed raw but blocked by `.gitignore`
**What goes wrong:** `mdCAD.exe` or future runtime DLLs never make it into git, so the consumer output copy silently misses files.

**Why it happens:** The repo globally ignores `*.exe`, `*.dll`, etc.

**How to avoid:** Add explicit allow-list rules for the chosen runtime-bundle directory, or the bundle will not be source-controlled.

**Warning signs:** Local build works from an untracked runtime folder; clean clone cannot run the control.

### Pitfall 6: Sealed mode accidentally grows into a second host product shell
**What goes wrong:** The control API/UI starts absorbing sample-harness/debug workflow scope.

**Why it happens:** Diagnostic chrome already exists and is tempting to expose more broadly.

**How to avoid:** Keep `sealed` bare, `diagnostic` explicit, and all richer orchestration/IPC ideas out of scope.

**Warning signs:** New command/event APIs to drive a running viewer; multiple control-specific status events being proposed.

## Code Examples

Verified patterns from official sources and current repo seams:

### Bindable control properties with Avalonia `StyledProperty`
```csharp
// Source: https://docs.avaloniaui.net/docs/guides/custom-controls/defining-properties
public static readonly StyledProperty<bool> AutoStartProperty =
    AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(AutoStart), defaultValue: true);
```

### `NativeControlHost` override seam
```csharp
// Source: https://api-docs.avaloniaui.net/docs/T_Avalonia_Controls_NativeControlHost
protected override IPlatformHandle CreateNativeControlCore(IPlatformHandle parent)
{
    IntPtr placeholder = Win32NativeMethods.CreatePlaceholderWindow(parent.Handle);
    return new PlatformHandle(placeholder, "HWND");
}
```

### Transitive output copy through SDK content items
```xml
<!-- Source: https://learn.microsoft.com/en-us/visualstudio/msbuild/common-msbuild-project-items?view=vs-2022 -->
<!-- Also verified locally in C:\Program Files\dotnet\sdk\9.0.306\Microsoft.Common.CurrentVersion.targets -->
<Content Include="runtime\win-x64\**\*">
  <Link>mdcad-runtime\%(RecursiveDir)%(Filename)%(Extension)</Link>
  <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
  <CopyToPublishDirectory>PreserveNewest</CopyToPublishDirectory>
</Content>
```

### Existing mdCAD launch argument pattern
```csharp
// Source: samples/avalonia-host/MainWindow.axaml.cs
startInfo.ArgumentList.Add("--embedded");
startInfo.ArgumentList.Add("--parent-hwnd");
startInfo.ArgumentList.Add($"0x{_launchParentHwnd.ToInt64():X}");
if (!string.IsNullOrWhiteSpace(resolvedJsonlPath))
{
    startInfo.ArgumentList.Add("--jsonl");
    startInfo.ArgumentList.Add(resolvedJsonlPath);
}
if (startupLiveRefreshEnabled && !string.IsNullOrWhiteSpace(resolvedJsonlPath))
{
    startInfo.ArgumentList.Add("--jsonl-live-refresh");
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| Sample app walks up to repo root and looks for `build-vulkan/bin/Release/mdCAD.exe` | Reusable control resolves a copied runtime folder under `AppContext.BaseDirectory` | Phase 48 recommendation | External consumers no longer depend on repo layout. |
| One-off sample window owns all embed logic directly | Reusable control owns lifecycle; sample app becomes diagnostic consumer | Phase 48 recommendation | Cross-repo reuse without losing the proof harness. |
| Ad hoc resource copy only for `resources/examples/**` | Full pinned runtime folder copied through MSBuild content metadata | Phase 48 recommendation | Consumer build output becomes self-sufficient for launch. |
| Immediate UI experimentation in a sample window | Fixed `sealed`/`diagnostic` modes with bindable startup props | Phase 48 recommendation | Public surface stays narrow and productizable. |

**Deprecated / outdated for this phase:**
- Repo-root executable discovery (`FindRepoRoot`) — incompatible with the local-project-reference goal.
- Copying only `mdCAD.exe` — contradicts the locked full-runtime-folder decision and is fragile for future native dependencies.
- Building mdCAD as part of the consuming app build — contradicts the pinned runtime-bundle decision.

## Open Questions

1. **How should the repo refresh the pinned runtime bundle when mdCAD changes?**
   - What we know: The consuming app must not build mdCAD from source, and `build-vulkan/bin/Release` is not a shippable folder because it includes test executables.
   - What's unclear: Whether Phase 48 should also add a maintainer-only bundle refresh script or document a manual curation step.
   - Recommendation: Plan for an explicit refresh seam, but keep it maintainer-only and separate from the consumer build path.

2. **Where should the new library live?**
   - What we know: It should be referenceable cross-repo and remain close to the current sample host assets.
   - What's unclear: `samples/` vs a new top-level `src-dotnet/` style folder.
   - Recommendation: Prefer `samples/avalonia-mdcad-control/` for this phase because it minimizes movement, keeps the harness nearby, and fits current repo organization.

3. **Should Phase 48 add a managed unit-test project?**
   - What we know: There is no existing .NET test project; true child-HWND attach/focus proof remains manual/UAT-heavy.
   - What's unclear: Whether the relaunch coordinator/path-normalization logic will be extracted enough to justify unit tests immediately.
   - Recommendation: Treat managed tests as a Wave 0 gap only for extracted pure logic seams; keep HWND/focus/orphan proof manual.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| .NET SDK | Avalonia library + sample consumer build | ✓ | 10.0.300 | — |
| CMake | Native mdCAD build / pinned bundle refresh | ✓ | 4.3.2 | Use existing built artifact only if native rebuild is unnecessary |
| CTest | Native regression suite | ✓ | 4.3.2 | — |
| Vulkan SDK | Windows native `build-vulkan` lane | ✓ | `C:\VulkanSDK\1.4.341.1` | None if native rebuild is part of validation |
| Existing native artifact | Initial pinned bundle materialization | ✓ | `build-vulkan/bin/Release/mdCAD.exe` present, last written 2026-05-15 | Rebuild native lane if artifact goes missing |

**Missing dependencies with no fallback:**
- None found.

**Missing dependencies with fallback:**
- None found.

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Native framework | CTest via existing CMake/MSVC Vulkan lane |
| Managed framework | None currently — build-only validation unless a new pure-logic test project is added |
| Quick run command | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` |
| Full suite command | `cmake --build build-vulkan --config Release && ctest --test-dir build-vulkan -C Release --output-on-failure && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| P48-01 | Reusable control library builds as a Windows-only project | build | `dotnet build samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj -c Release` | ❌ Wave 0 |
| P48-02 | Consuming app receives `mdcad-runtime/**` under its output via `ProjectReference` | smoke | `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` + output folder/file existence check | ❌ Wave 0 |
| P48-03 | XAML-dropped control auto-launches and attaches mdCAD inside the hosted region | manual/UAT | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ existing harness, new checklist needed |
| P48-04 | `AutoStart=false` plus explicit start works without orphaning mdCAD | manual/UAT | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` + `Get-Process mdCAD -ErrorAction SilentlyContinue` | ✅ harness, ❌ checklist |
| P48-05 | Changing `JsonlPath` / live-refresh while running triggers one safe relaunch using the latest snapshot only | manual/UAT + optional unit | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ harness, ❌ managed pure-logic tests |
| P48-06 | `sealed` mode stays bare, `diagnostic` mode shows host-owned controls/status, invalid requested JSONL stays visible | manual/UAT | `dotnet run --project samples/avalonia-host/AvaloniaHost.csproj -c Release` | ✅ harness, ❌ checklist |
| P48-07 | Native mdCAD launch contracts from Phases 43-47 still pass | regression | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ |

### Sampling Rate
- **Per task commit:** `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Per wave merge:** `cmake --build build-vulkan --config Release && ctest --test-dir build-vulkan -C Release --output-on-failure && dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Phase gate:** Full suite green plus manual/UAT checklist pass for attach/relaunch/mode/warning/orphan scenarios before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` — reusable control project does not exist yet
- [ ] Managed test seam for any extracted pure logic (`MdCadLaunchSnapshot`, path normalization, relaunch coordinator coalescing) — no .NET test project exists yet
- [ ] Phase 48 manual checklist — must extend Phase 47 scenarios to include runtime-subfolder copy, auto-start, explicit start, sealed/diagnostic, and property-change relaunch
- [ ] Output-copy smoke check — add a deterministic file/folder existence assertion for `mdcad-runtime\mdCAD.exe` under the consuming app output

## Sources

### Primary (HIGH confidence)
- `C:\dev\mdCAD\.planning\phases\48-reusable-avalonia-mdcad-user-control\48-CONTEXT.md` — locked scope and canonical refs
- `C:\dev\mdCAD\.planning\ROADMAP.md` — v1.8 boundary and Phase 48 placement
- `C:\dev\mdCAD\.planning\REQUIREMENTS.md` — no-IPC / Windows-only / existing embed requirements
- `C:\dev\mdCAD\.planning\STATE.md` — prior phase decisions that must carry forward
- `C:\dev\mdCAD\samples\avalonia-host\AvaloniaHost.csproj` — current Avalonia packages and content-copy pattern
- `C:\dev\mdCAD\samples\avalonia-host\MainWindow.axaml` — current diagnostic/status layout
- `C:\dev\mdCAD\samples\avalonia-host\MainWindow.axaml.cs` — placeholder HWND, attach detection, teardown, path resolution, relaunch
- `C:\dev\mdCAD\src\app_launch_config.h` — launch contract
- `C:\dev\mdCAD\src\embed_layout_state.h` and `C:\dev\mdCAD\src\imgui_storage.h` — working-directory-relative embedded layout persistence
- `C:\Program Files\dotnet\sdk\9.0.306\Microsoft.Common.CurrentVersion.targets` (esp. `GetCopyToOutputDirectoryItems` sections) — SDK transitive content-copy behavior for project references
- https://docs.avaloniaui.net/docs/guides/custom-controls/defining-properties — Avalonia `StyledProperty` guidance
- https://docs.avaloniaui.net/docs/guides/custom-controls/how-to-create-advanced-custom-controls — `TemplatedControl`/custom-control guidance
- https://api-docs.avaloniaui.net/docs/T_Avalonia_Controls_NativeControlHost — `NativeControlHost` API seam
- https://learn.microsoft.com/en-us/visualstudio/msbuild/common-msbuild-project-items?view=vs-2022 — `Content`, `CopyToOutputDirectory`, `TargetPath`
- https://learn.microsoft.com/en-us/dotnet/core/project-sdk/msbuild-props#copytopublishdirectory — `CopyToPublishDirectory` and `LinkBase`

### Secondary (MEDIUM confidence)
- `dotnet list samples/avalonia-host/AvaloniaHost.csproj package` — resolved Avalonia package versions
- NuGet registration data for `Avalonia`, `Avalonia.Desktop`, `Avalonia.Themes.Fluent` — current/published package metadata

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — current repo packages were resolved locally and official package metadata/docs were checked
- Architecture: MEDIUM — driven by strong repo evidence plus official Avalonia/MSBuild docs, but the exact extracted project shape is still a recommendation
- Pitfalls: HIGH — mostly derived from already-verified Phases 43-47 behavior and direct code inspection

**Research date:** 2026-05-15
**Valid until:** 2026-06-14
