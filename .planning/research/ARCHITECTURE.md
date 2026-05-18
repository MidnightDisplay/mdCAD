# Architecture Research: Plain `net10.0` Avalonia Host Compatibility

**Milestone:** v1.9  
**Scope:** architecture only for adding plain `net10.0` host compatibility without changing the proven Windows embedded-runtime seam  
**Confidence:** MEDIUM-HIGH

## Current repo shape

Today the reusable control project mixes two responsibilities:

1. **Host-facing Avalonia surface**
   - `MdCadEmbeddedControl.axaml`
   - `MdCadEmbeddedControl.axaml.cs`

2. **Windows-only runtime ownership**
   - `Host/EmbedNativeControlHost.cs`
   - `Host/MdCadRuntimeResolver.cs`
   - child-HWND placeholder/attach, resize sync, runtime bundle lookup, process launch

That was fine for v1.8 when every consumer targeted `net10.0-windows10.0.19041.0`, but it is the reason plain `net10.0` hosts currently fail at restore.

## Architectural options

### Option 1 — Retarget the existing control project to `net10.0` and keep the current structure

**Pros**
- Smallest diff
- Directly removes the TFM compatibility wall

**Cons**
- Leaves Win32/runtime ownership smeared through shared control code
- Keeps non-Windows behavior implicit instead of intentional

**Verdict:** technically viable, but not the cleanest shape for v1.9.

### Option 2 — **Recommended**: one public project, internal backend split

Keep one public project/assembly: `MdCad.Avalonia.Control`, but split its internals into:

```text
MdCadEmbeddedControl (shared shell / public API)
  -> IMdCadEmbedBackend (internal)
      -> WindowsMdCadEmbedBackend
      -> UnsupportedMdCadEmbedBackend
```

**Why this fits best**
- Plain `net10.0` hosts get a compatible reference path
- The proven Windows runtime seam stays explicit and library-owned
- Existing XAML namespace / assembly identity stays unchanged
- Non-Windows behavior becomes deliberate, not accidental

### Option 3 — Split into two public projects

For example:
- `MdCad.Avalonia.Control` (`net10.0`)
- `MdCad.Avalonia.Control.Windows` (`net10.0-windows...`)

**Verdict:** not recommended for v1.9 because it complicates direct consumer onboarding and packaging without solving a problem this milestone actually needs to solve.

## Recommended architecture

### Public surface

Keep the current public contract:

- `MdCadEmbeddedControl`
- `JsonlPath`
- `StartupLiveRefreshEnabled`
- `AutoStart`
- `PresentationMode`
- `StartAsync()`
- `StopAsync()`

Do **not** add public Windows-specific knobs in this milestone.

### Shared shell

`MdCadEmbeddedControl` should own only:
- styled properties
- warning/diagnostic UI
- presentation mode switching
- coordinator/backend wiring

It should stop owning directly:
- placeholder HWND creation
- child attach polling
- resize sync
- runtime resolution
- process launch details

### Windows backend

Own:
- `NativeControlHost` placeholder creation
- `user32.dll` P/Invokes
- runtime bundle resolution from `AppContext.BaseDirectory\\mdcad-runtime`
- `mdCAD.exe --embedded --parent-hwnd ...` launch
- attach polling / child HWND checks
- resize sync and teardown

### Unsupported backend

Own:
- non-Windows inert surface
- warning text such as `Embedded mdCAD runtime is currently supported only on Windows.`
- never-start behavior

It should never attempt:
- runtime resolution
- Windows path validation for launch
- process launch
- placeholder HWND logic

## Important boundary correction

`MdCadLaunchSnapshot` is currently too Windows-shaped to stay fully shared.

Recommended:
- shared layer: `MdCadLaunchRequest` (raw host intent)
- Windows backend: normalize into Windows-specific launch arguments
- Unsupported backend: ignore launch normalization beyond basic host intent and show the platform warning

This avoids misleading non-Windows users with Windows-only path diagnostics.

## New vs modified surfaces

### New

- `Host/IMdCadEmbedBackend.cs`
- `Host/MdCadEmbedBackendFactory.cs`
- `Host/UnsupportedMdCadEmbedBackend.cs`
- `Host/Windows/WindowsMdCadEmbedBackend.cs`
- optionally `Host/MdCadLaunchRequest.cs`

### Modified

- `MdCadEmbeddedControl.axaml.cs` → shared shell/orchestrator only
- `MdCadEmbeddedControl.axaml` → explicit unsupported-platform warning behavior
- `MdCad.Avalonia.Control.csproj` → `net10.0`
- `Host/MdCadSessionCoordinator.cs` → backend-neutral lifecycle orchestration
- `Host/EmbedNativeControlHost.cs` → Windows backend helper
- `Host/MdCadRuntimeResolver.cs` → Windows backend only
- `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` → plain `net10.0`
- `QUICKSTART.md` → compile-time vs runtime contract wording

## Safe rollout / phase order

### Phase 1 — Extract backend seam without changing behavior

- Introduce `IMdCadEmbedBackend`
- Move Win32/runtime/process logic behind a Windows backend
- Keep current Windows behavior identical

### Phase 2 — Add unsupported-platform backend + explicit warning contract

- Add inert backend
- Make non-Windows behavior intentional and visible
- Stop the shared shell from using Windows-only readiness/path logic directly

### Phase 3 — Retarget the consumer-facing control to plain `net10.0`

- Change `MdCad.Avalonia.Control.csproj` to `net10.0`
- Move shared tests to the new host-facing contract

### Phase 4 — Add the plain `net10.0` consumer proof

- Retarget `samples/avalonia-host-minimal` to plain `net10.0`
- Keep `samples/avalonia-host` as the Windows diagnostic harness

### Phase 5 — Docs and packaging cleanup

- Update `QUICKSTART.md`
- Update README wording
- Make compile-time vs runtime support matrix explicit

## Biggest pitfall to avoid

Do **not** solve this by merely changing the TFM while leaving all Windows semantics smeared across shared control code.

That would remove the `NU1201` blocker but leave the runtime boundary unclear and unsupported-platform behavior under-specified.
