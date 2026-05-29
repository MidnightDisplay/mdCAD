# Phase 56: Create WPF mdCAD control for net10.0-windows consumers with Avalonia-parity parameters plus full and minimal sample test projects - Research

**Researched:** 2026-05-28  
**Domain:** WPF child-HWND hosting for the existing mdCAD external-process embed seam  
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

All items below are copied verbatim from the phase context artifact. [CITED: .planning/phases/56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-/56-CONTEXT.md]

### Locked Decisions
### WPF control contract parity
- **D-01:** The WPF control should expose the same host-facing configuration surface as the Avalonia control wherever WPF idioms allow direct parity.
- **D-02:** The parameter/property set must include `JsonlPath`, `StartupLiveRefreshEnabled`, `ViewportOnlyStartupMode`, `AutoStart`, and `PresentationMode`.
- **D-03:** The public control API must keep explicit `StartAsync()` and `StopAsync()` entry points in addition to `AutoStart`.
- **D-04:** Launch-affecting setting changes after session start should remain control-managed relaunches using the newest snapshot rather than leaving restart orchestration to the host app.

### Windows runtime and embedding boundary
- **D-05:** The WPF control remains Windows-only at runtime and must continue using the existing child-HWND launch contract for mdCAD.
- **D-06:** The control must keep the committed runtime-bundle model and copy `mdcad-runtime/**` into the consumer output just like the Avalonia control.
- **D-07:** Unsupported-runtime messaging must stay truthful; this phase does not expand mdCAD runtime support beyond Windows.

### Sample proof surfaces
- **D-08:** The repository should include both a full diagnostic WPF sample host and a minimal sealed WPF sample host, mirroring the Avalonia proof split.
- **D-09:** The full WPF sample is the richer lifecycle/status harness; the minimal WPF sample is the smallest consumer/onboarding proof.
- **D-10:** WPF sample defaults and docs/test artifacts should align with the existing compile-time-vs-runtime truth established in the Avalonia milestone.

### The agent's discretion
- Choose the exact WPF hosting primitive (`HwndHost`, composition layout, helper classes) as long as it preserves the proven child-HWND attach/recreate/teardown behavior.
- Choose the exact project names/namespaces for the WPF control and WPF sample apps, as long as they remain consistent with repo conventions and clearly parallel the Avalonia artifacts.
- Choose the exact split between shared host-agnostic lifecycle code and WPF-specific shell code, as long as parity stays tight and duplication stays controlled.
- Choose the exact WPF test strategy and project layout, as long as the new phase locks the public contract and sample-proof surfaces credibly.

### Deferred Ideas (OUT OF SCOPE)
- Broader docs/onboarding refresh unless the new WPF samples force immediate truthfulness updates
- New distribution/package strategy spanning both Avalonia and WPF controls
- Additional host-facing capabilities beyond strict parity with the existing Avalonia control
- Multi-control orchestration, richer diagnostics IPC, or cross-host abstraction cleanup beyond what Phase 56 needs
</user_constraints>

## Project Constraints (from copilot-instructions.md)

None — `copilot-instructions.md` is not present in the repo root during this research pass. [ASSUMED]

## Summary

Use a new WPF `UserControl` whose native viewer region is implemented with a dedicated `HwndHost` subclass, while reusing the Avalonia control’s framework-neutral launch snapshot, runtime resolution, presentation enum, session coordination, and runtime-refresh tooling instead of cloning all host logic. Microsoft documents `HwndHost` as the standard WPF mechanism for hosting a Win32 child window and requires `BuildWindowCore` / `DestroyWindowCore`; the existing Avalonia control already isolates most non-visual lifecycle logic behind host classes that can be shared. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf] [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs]

Do **not** widen this into a generic cross-framework UI abstraction or a second runtime-packaging pipeline. The right split is: shared host core for snapshot/reconcile/runtime resolution, WPF-specific shell for dependency properties and `HwndHost`, and one canonical committed Windows runtime payload copied into `mdcad-runtime/` for consumers. The existing Avalonia control project plus runtime-refresh tests already prove the current packaging contract; Phase 56 should preserve that contract instead of forking it. [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshHelperTests.cs]

The most important WPF-specific warning is the HWND airspace/input boundary: `HwndHost` content renders above normal WPF content, does not respect normal clipping in the same way, and WPF mouse/keyboard events do not flow while focus is inside the hosted HWND. That means the diagnostic chrome must stay **outside** the native viewer rectangle, not overlaid on top of it, and host status must remain host-owned rather than pretending to observe in-process mdCAD state. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/wpf-and-win32-interoperation] [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml] [CITED: samples/avalonia-host/MainWindow.axaml.cs]

**Primary recommendation:** Build a Windows-only WPF control library on `net10.0-windows` with `UseWPF=true`, back it with a custom `HwndHost`, share the Avalonia control’s non-visual host logic, keep one canonical `mdcad-runtime` payload, and mirror the existing full/minimal sample split without introducing IPC or a second runtime toolchain. [CITED: https://learn.microsoft.com/en-us/dotnet/core/project-sdk/msbuild-props-desktop] [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|---|---|---|---|
| Public bindable control API (`JsonlPath`, `AutoStart`, `PresentationMode`, methods) | WPF shell | Shared host core | WPF owns dependency properties and XAML consumption; the shared core should only consume snapshots and commands. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/properties/custom-dependency-properties] [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs] |
| Placeholder HWND creation/destruction | WPF shell | Win32 helper layer | `HwndHost` is the WPF seam that owns child HWND lifetime. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf] [CITED: samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs] |
| Launch snapshot normalization and relaunch coalescing | Shared host core | WPF shell | The existing snapshot/coordinator logic is UI-framework-agnostic and should stay so. [CITED: samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs] |
| mdCAD process launch / attach / resize / teardown | Windows backend | Shared host core | The backend stays Windows-specific because it owns HWND/process behavior, but it should be driven by shared snapshots and coordinator rules. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] |
| Runtime bundle copy and resolution | MSBuild + runtime resolver | Sample hosts | The control project copies `mdcad-runtime/**`; hosts only verify presence and consume it from output. [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj] [CITED: samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs] [CITED: samples/avalonia-host/MainWindow.axaml.cs] |
| Viewer rendering and in-viewer input | External mdCAD process | — | mdCAD remains a separate process attached as a child HWND; the host must not claim in-process rendering/input ownership. [CITED: .planning/phases/56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-/56-CONTEXT.md] [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] |
| Rich lifecycle proof surface | Full WPF sample host | Control library | The full sample should remain the place for host-owned status, scenario switches, and manual proof. [CITED: samples/avalonia-host/MainWindow.axaml] [CITED: samples/avalonia-host/MainWindow.axaml.cs] |
| Smallest consumer/onboarding proof | Minimal WPF sample host | Control library | The minimal sample should only prove reference, XAML instantiation, and simple binding. [CITED: samples/avalonia-host-minimal/MainWindow.axaml] [CITED: samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs] |

## Reusable Assets

| Asset | Where | Reuse Guidance |
|---|---|---|
| Launch snapshot normalization and warning semantics | `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs` | Reuse directly or move to a shared host-core folder; this already encodes the locked `JsonlPath` + live-refresh + viewport-only snapshot semantics. [CITED: samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs] |
| Serialized start/stop/relaunch coordinator | `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` | Reuse directly or extract with minimal edits; it already coalesces launch-affecting changes and recreates the native surface before restart. [CITED: samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs] |
| Presentation enum | `samples/avalonia-mdcad-control/Host/MdCadPresentationMode.cs` | Share as-is so Avalonia and WPF keep identical public mode names and defaults. [CITED: samples/avalonia-mdcad-control/Host/MdCadPresentationMode.cs] |
| Runtime output resolution | `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs` | Share as-is; the WPF control should still resolve `AppContext.BaseDirectory\\mdcad-runtime\\mdCAD.exe`. [CITED: samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs] |
| Start-info argument order and working-directory rules | `WindowsMdCadEmbedBackend.CreateStartInfo(...)` behavior locked by tests | Reuse the argument-construction logic exactly so WPF forwards `--embedded`, `--parent-hwnd`, `--viewport-only`, `--jsonl`, and `--jsonl-live-refresh` identically. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] |
| Win32 placeholder helpers | `samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs` | Reuse the existing P/Invoke helpers; only the control-host wrapper should become WPF-specific. [CITED: samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs] |
| Runtime refresh helper | `samples/avalonia-mdcad-control/tools/MdCad.WindowsRuntimeRefresh/` | Reuse this exact helper rather than creating a WPF-only refresh tool. [CITED: samples/avalonia-mdcad-control/tools/MdCad.WindowsRuntimeRefresh/Program.cs] [CITED: samples/avalonia-mdcad-control/tools/MdCad.WindowsRuntimeRefresh/MdCad.WindowsRuntimeRefresh.csproj] |
| Diagnostic host UX semantics | `samples/avalonia-host/MainWindow.axaml` and `.cs` | Mirror the same host-owned runtime status, harness status, JSONL scenario switches, and explicit `StartAsync` / `StopAsync` buttons in the full WPF sample. [CITED: samples/avalonia-host/MainWindow.axaml] [CITED: samples/avalonia-host/MainWindow.axaml.cs] |
| Minimal consumer shape | `samples/avalonia-host-minimal/` | Mirror the same “single control in XAML with bound `JsonlPath`” approach in the minimal WPF sample. [CITED: samples/avalonia-host-minimal/MainWindow.axaml] [CITED: samples/avalonia-host-minimal/MainWindow.axaml.cs] [CITED: samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs] |

## Recommended Phase Split

1. **Plan 56-01 — Extract/share the non-visual host core and lock parity tests first.** Rehome `MdCadLaunchSnapshot`, `MdCadSessionCoordinator`, `MdCadRuntimeResolver`, `MdCadPresentationMode`, and any start-info helper into a location both controls can consume, then add/port tests that lock parity for snapshot semantics, relaunch coalescing, runtime resolution, and argument forwarding before WPF UI work begins. [CITED: samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs] [CITED: samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs] [CITED: samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs] [CITED: samples/avalonia-mdcad-control.tests/MdCadRuntimeResolverTests.cs] [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs]

2. **Plan 56-02 — Implement the WPF control and backend on top of that shared core.** Create a WPF class-library project targeting `net10.0-windows` with `UseWPF=true`, expose dependency properties plus `StartAsync` / `StopAsync`, build a custom `HwndHost` placeholder, and keep the current Windows process attach/resize/teardown model behind a WPF-specific backend rather than forcing a shared visual abstraction. [CITED: https://learn.microsoft.com/en-us/dotnet/core/project-sdk/msbuild-props-desktop] [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf] [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/properties/custom-dependency-properties] [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs] [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]

3. **Plan 56-03 — Add full/minimal WPF consumers and close proof/packaging.** Add a diagnostic WPF sample host that mirrors the Avalonia harness, add a minimal WPF sample host that mirrors the minimal Avalonia consumer, point the WPF control at the same canonical committed runtime bundle, and close with build/output verification plus a Windows manual lifecycle proof. [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs] [CITED: samples/avalonia-host/MainWindow.axaml] [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/avalonia-host-minimal/MainWindow.axaml]

## Stack / Tooling Guidance

### Core project settings

- Use `Microsoft.NET.Sdk` with `TargetFramework` set to a Windows-specific TFM and `UseWPF=true`; Microsoft’s desktop SDK guidance explicitly documents this as the supported WPF project shape. For this phase, prefer `net10.0-windows` to match the user-stated consumer target unless a concrete Windows-version-specific API forces a narrower TFM. [CITED: https://learn.microsoft.com/en-us/dotnet/core/project-sdk/msbuild-props-desktop] [ASSUMED]
- Make the reusable control a library project, not `WinExe`; reserve `OutputType=WinExe` for the sample hosts only. [CITED: https://learn.microsoft.com/en-us/dotnet/core/project-sdk/msbuild-props-desktop]
- Do **not** add a second UI toolkit or a WinForms bridge just to get an HWND. `HwndHost` is the native WPF interop seam for hosting a Win32 child window. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf]

### Public API surface

- Implement the WPF public surface with dependency properties plus CLR wrappers for `JsonlPath`, `StartupLiveRefreshEnabled`, `ViewportOnlyStartupMode`, `AutoStart`, and `PresentationMode`; Microsoft’s dependency-property guidance explicitly calls out this wrapper pattern for WPF properties that need XAML binding/styling semantics. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/properties/custom-dependency-properties]
- Keep `StartAsync()` and `StopAsync()` as ordinary public methods on the control, wired into the same coordinator semantics the Avalonia control already uses. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs]

### Runtime packaging

- Preserve the existing `mdcad-runtime` copy contract with SDK-style `Content` items and `Link`/`CopyToOutputDirectory` metadata. The Avalonia control project already does this and the integration tests verify that the consumer output resolves through `mdcad-runtime`. [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs]
- Reuse the existing `MdCad.WindowsRuntimeRefresh` helper if the WPF control also needs an opt-in post-build refresh path. Do not create a second native-refresh helper. [CITED: samples/avalonia-mdcad-control/tools/MdCad.WindowsRuntimeRefresh/Program.cs] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshHelperTests.cs]
- Prefer one canonical committed runtime source directory shared by both controls over duplicating `runtime/win-x64` in two places. That keeps the runtime hash/proof surface single-sourced. [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs] [ASSUMED]

### Testing/tooling

- Stay on the repo’s existing xUnit lane for unit/integration coverage unless a concrete WPF-only gap forces a new tool. The current control tests already use `xunit`, `Microsoft.NET.Test.Sdk`, and `coverlet.collector`. [CITED: samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj]
- Prefer build/output assertions and shared host-core tests over heavyweight WPF UI automation packages; the critical risks in this phase are contract parity, runtime copy, and relaunch ordering, not pixel-perfect WPF rendering. [CITED: samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs] [CITED: samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs] [ASSUMED]

## Architecture Patterns

### Pattern 1: WPF shell outside, native HWND inside

Use a normal WPF `UserControl` for warning/diagnostic chrome and reserve one child region for a custom `HwndHost` that owns the placeholder HWND. Microsoft documents `HwndHost` as the control-like wrapper for a Win32 child window in WPF, and the current Avalonia control already uses the same visual separation with warnings/diagnostics above the native embed surface. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf] [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml]

```csharp
// Source pattern: Microsoft HwndHost docs + current EmbedNativeControlHost/Win32NativeMethods split
public sealed class MdCadHwndHost : HwndHost
{
    public IntPtr PlaceholderHandle { get; private set; }

    protected override HandleRef BuildWindowCore(HandleRef hwndParent)
    {
        var hwnd = Win32NativeMethods.CreatePlaceholderWindow(hwndParent.Handle);
        PlaceholderHandle = hwnd;
        return new HandleRef(this, hwnd);
    }

    protected override void DestroyWindowCore(HandleRef hwnd)
    {
        if (hwnd.Handle != IntPtr.Zero)
        {
            Win32NativeMethods.DestroyWindow(hwnd.Handle);
        }

        PlaceholderHandle = IntPtr.Zero;
    }
}
```
[CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf] [CITED: samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs] [CITED: samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs]

### Pattern 2: Shared host core, framework-specific shell

Share the launch snapshot, coordinator, runtime resolver, presentation enum, and runtime-refresh helper; keep WPF-specific code limited to dependency properties, XAML, `HwndHost`, and the WPF backend surface. The current Avalonia control already demonstrates that most lifecycle rules are not UI-framework-specific. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs]

### Pattern 3: Preserve process boundary; do not invent message-based host insight

Keep the existing process-launched CLI contract and attach detection model. The current backend starts mdCAD with `--embedded`, `--parent-hwnd`, optional `--viewport-only`, optional `--jsonl`, and optional `--jsonl-live-refresh`; WPF interop docs also state that `HwndHost.WndProc` cannot process messages from out-of-process windows, which matches the existing “host-owned status only” design. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs] [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/wpf-and-win32-interoperation] [CITED: samples/avalonia-host/MainWindow.axaml.cs]

### Pattern 4: One runtime payload, many consumers

Keep `mdCAD.exe` and `imgui.embedded.ini` in one committed canonical runtime bundle and copy that bundle into each consuming app’s output. The current runtime resolver and integration tests depend on that contract. [CITED: samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs]

### Pattern 5: Mirror the sample split exactly

The full WPF host should port the existing harness responsibilities: runtime-presence line, harness-status line, presentation-mode toggle, JSONL scenario selector, live-refresh toggle, and explicit `StartAsync` / `StopAsync` actions. The minimal WPF host should stay a single-window, single-control onboarding proof with a simple viewmodel-bound `JsonlPath`. [CITED: samples/avalonia-host/MainWindow.axaml] [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/avalonia-host-minimal/MainWindow.axaml] [CITED: samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs]

## Testing / Proof Recommendations

### Automated

- Port or reuse the launch-snapshot tests so WPF parity keeps absolute-path validation, missing/unreadable warning behavior, and `ViewportOnlyStartupMode` capture exactly aligned with Avalonia. [CITED: samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs]
- Port or reuse the session-coordinator tests so WPF keeps auto-start gating, explicit start, restart-on-launch-change, coalescing, and placeholder recreation guarantees. [CITED: samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs]
- Lock the Windows start-info contract with tests equivalent to the Avalonia backend tests, especially arg order and working directory. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs]
- Add a WPF-specific build/output proof that a consumer build produces `mdcad-runtime\\mdCAD.exe` and preserves `imgui.embedded.ini`. The existing Avalonia integration test is the model. [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs]
- Keep the minimal WPF sample as a build proof, not a second diagnostic harness. [CITED: samples/avalonia-host-minimal/MainWindow.axaml]

### Manual Windows proof

- Full WPF sample: verify copied runtime presence, attach success, explicit stop, relaunch after changing `JsonlPath`, relaunch after toggling `StartupLiveRefreshEnabled`, relaunch after toggling `ViewportOnlyStartupMode`, and truthful warnings for missing/unreadable startup paths. [CITED: samples/avalonia-host/MainWindow.axaml.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs]
- Minimal WPF sample: verify clean build/reference, control instantiation from XAML, default sealed presentation, and startup with bound `JsonlPath`. [CITED: samples/avalonia-host-minimal/MainWindow.axaml] [CITED: samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs]

### Proposed commands

- `dotnet test samples\\wpf-mdcad-control.tests\\MdCad.Wpf.Control.Tests.csproj -c Release` for shared/WPF unit coverage. [ASSUMED]
- `dotnet build samples\\wpf-host\\WpfHost.csproj -c Release` for the diagnostic consumer proof. [ASSUMED]
- `dotnet build samples\\wpf-host-minimal\\WpfHostMinimal.csproj -c Release` for the smallest consumer proof. [ASSUMED]

## Common Pitfalls

### Pitfall 1: Over-abstracting the UI stack
Trying to force Avalonia and WPF to share the same visual host surface will inflate scope and mix incompatible control models. Share the non-visual host core only; keep `UserControl`/dependency-property/XAML work WPF-specific. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs] [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/properties/custom-dependency-properties]

### Pitfall 2: Treating `HwndHost` like a normal WPF visual
`HwndHost` content appears above normal WPF elements, has clipping/transform limitations, and WPF mouse/keyboard events do not flow while focus is inside the hosted HWND. Keep all warning/diagnostic chrome outside the native viewer rectangle and do not rely on WPF overlay behavior. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/wpf-and-win32-interoperation]

### Pitfall 3: Inventing out-of-process message interception
Microsoft explicitly documents that `HwndHost.WndProc` cannot process messages from windows that are out of process. mdCAD is an external process, so Phase 56 should keep attach polling and host-owned status text instead of attempting a richer message bridge. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/wpf-and-win32-interoperation] [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs] [CITED: samples/avalonia-host/MainWindow.axaml.cs]

### Pitfall 4: Forking the runtime payload
Creating a second committed runtime bundle or a second refresh helper will make the Avalonia and WPF controls drift. Reuse the current runtime folder/tooling or extract one shared runtime root; do not duplicate the payload. [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs] [ASSUMED]

### Pitfall 5: Re-running mdCAD directly from every property callback
The existing control already proves that launch-affecting changes must go through one serialized reconcile path. Repeating that logic ad hoc in WPF property-changed callbacks will reintroduce orphan/race bugs. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs] [CITED: samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs]

## Explicit Code Anchors

- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` — exact public API to mirror, current coordinator wiring, and current property-change behavior. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs]
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml` — current sealed-vs-diagnostic layout split to mirror in WPF. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml]
- `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs` — startup snapshot truth for `JsonlPath`, live refresh, viewport-only, and warning text. [CITED: samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs]
- `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` — start/stop/relaunch ordering, auto-start suppression, and surface recreation sequencing. [CITED: samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs]
- `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs` — canonical `AppContext.BaseDirectory\\mdcad-runtime` resolution rule. [CITED: samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs]
- `samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs` — closest code analogue for the new WPF `HwndHost`. [CITED: samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs]
- `samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs` — reusable placeholder/child-window Win32 helpers. [CITED: samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs]
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` — attach timer, resize sync, lifecycle teardown, and backend callback surface to port/adapt. [CITED: samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs]
- `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` — arg-order and working-directory lock for WPF backend parity. [CITED: samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs]
- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` — current runtime copy target and opt-in runtime-refresh target. [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj]
- `samples/avalonia-host/MainWindow.axaml` and `MainWindow.axaml.cs` — full diagnostic harness behavior to mirror, not reinvent. [CITED: samples/avalonia-host/MainWindow.axaml] [CITED: samples/avalonia-host/MainWindow.axaml.cs]
- `samples/avalonia-host-minimal/MainWindow.axaml` and `ViewModels/MainWindowViewModel.cs` — minimal host shape to mirror in WPF. [CITED: samples/avalonia-host-minimal/MainWindow.axaml] [CITED: samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| `dotnet` | WPF projects, builds, tests | ✓ | `10.0.300` | — [ASSUMED] |
| `cmake` | Existing runtime-refresh helper build path | ✓ | `4.3.2` | — [ASSUMED] |
| `ninja` | Fresh native configure work only | ✗ | — | Reuse existing configured `build-vulkan` flow; Phase 56 does not need to add a new Ninja dependency. [ASSUMED] |
| `git` | Repo workflow | ✓ | `2.51.1.windows.1` | — [ASSUMED] |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|---|---|---|
| A1 | `net10.0-windows` is the best default TFM for the new WPF projects unless a concrete Windows-version-specific API forces `net10.0-windows10.0.19041.0`. | Stack / Tooling Guidance | Medium — an unnecessary TFM narrowing could reintroduce host-compatibility friction. |
| A2 | A small internal STA test helper is sufficient if any WPF-specific control-construction tests are needed, so no new WPF test package is required initially. | Testing / Proof Recommendations | Low — the fallback is adding a scoped WPF-test dependency later. |
| A3 | The WPF control can and should consume one shared committed runtime source directory instead of owning a second runtime copy. | Stack / Tooling Guidance / Pitfalls | Medium — if path/layout assumptions block this, the plan must include a safe extraction step rather than duplication. |
| A4 | The environment probe versions captured during research should be treated as current-machine observations, not portable repo guarantees. | Environment Availability | Low — planner just needs them as local execution context. |

## Sources

### Primary
- https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf
- https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/wpf-and-win32-interoperation
- https://learn.microsoft.com/en-us/dotnet/core/project-sdk/msbuild-props-desktop
- https://learn.microsoft.com/en-us/dotnet/desktop/wpf/properties/custom-dependency-properties

### Codebase
- `.planning/phases/56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-/56-CONTEXT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/PROJECT.md`
- `.planning/STATE.md`
- `.planning/milestones/v1.8-phases/48-reusable-avalonia-mdcad-user-control/48-CONTEXT.md`
- `.planning/milestones/v1.8-phases/48-reusable-avalonia-mdcad-user-control/48-RESEARCH.md`
- `samples/avalonia-mdcad-control/**`
- `samples/avalonia-mdcad-control.tests/**`
- `samples/avalonia-host/**`
- `samples/avalonia-host-minimal/**`

## Metadata

**Confidence breakdown:**  
- WPF hosting approach: HIGH — directly supported by Microsoft WPF interop docs. [CITED: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/advanced/hosting-win32-content-in-wpf]  
- Shared-vs-WPF-specific split: MEDIUM — strongly supported by current repo structure, but exact extraction boundaries remain implementation choices. [CITED: samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs]  
- Runtime packaging recommendation: MEDIUM — current repo proves the packaging contract, but the one-runtime-source recommendation still needs implementation confirmation. [CITED: samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj] [CITED: samples/avalonia-mdcad-control.tests/RuntimeRefreshIntegrationTests.cs] [ASSUMED]
