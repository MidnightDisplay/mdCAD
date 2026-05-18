# STACK Research: Plain `net10.0` Avalonia Host Compatibility

**Project:** mdCAD  
**Milestone:** v1.9 Plain `net10.0` Avalonia Host Compatibility  
**Researched:** 2026-05-18  
**Confidence:** HIGH

## Recommendation

Make `MdCad.Avalonia.Control` a host-facing library targeting **`net10.0`**.

Keep the actual mdCAD embedding path **Windows-only by runtime guard**, not by exposing only a Windows-specific TFM. The Avalonia UI surface is already cross-platform; the real runtime boundary is the Win32 child-HWND attach/launch seam.

## Recommended stack pattern

| Item | Recommendation | Why |
|------|----------------|-----|
| Public control TFM | `net10.0` | Removes the current `NU1201` wall for plain `net10.0` hosts |
| UI packages | Keep `Avalonia` + `Avalonia.Desktop` | Existing package line is already compatible with plain `net10.0` consumers |
| Windows-only enforcement | `OperatingSystem.IsWindows()` guards plus platform annotations/analyzers | Keeps the runtime seam explicit without pretending embedding is cross-platform |
| Runtime payload | Keep explicit Windows payload only (`mdcad-runtime/win-x64`) | Truthful packaging: broad host contract, narrow runtime bundle |
| Proof hosts | Retarget the minimal compatibility proof to `net10.0`; keep the real attach/diagnostic harness Windows-focused | Separates compile-time proof from real Win32 runtime proof |

## Viable patterns

### 1. Single public target: `net10.0` — **Recommended**

- Retarget `MdCad.Avalonia.Control` to `net10.0`
- Keep Win32 interop in the same assembly
- Guard every Windows-only path at runtime

This directly fixes `ProjectReference` compatibility while preserving the proven Windows runtime seam.

### 2. Multi-target: `net10.0;net10.0-windows10.0.19041.0` — **Viable only if `net10.0` is complete**

This is useful only if the **`net10.0`** target is already the true public contract. A plain `net10.0` host will still bind to the base-TFM asset, so multi-targeting alone does not solve the milestone.

### 3. Split public assemblies (`Control` + `Control.Windows`) — **Not recommended for v1.9**

This is possible, but it complicates direct `ProjectReference` usage and packaging for little value in this milestone.

## Constraints that matter

### TFM compatibility

- A plain `net10.0` app or library cannot consume a dependency that only targets `net10.0-windows...`.
- The control must expose a **base `net10.0` asset** if plain `net10.0` hosts are meant to reference it.

### Runtime identifiers are not the fix

RID selection affects runtime assets, not compile-time/project compatibility. `RuntimeIdentifier=win-x64` does **not** fix `NU1201`.

### Platform analyzers matter

Use:

- `OperatingSystem.IsWindows()`
- `[SupportedOSPlatform("windows")]` on Windows-only internals
- CA1416 / platform compatibility analyzer guardrails

### Avalonia is not the blocker

Avalonia 11.3.x already restores on plain `net10.0`; the blocker is the control project's Windows-specific TFM, not the Avalonia package line.

## Concrete integration points

1. **`samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj`**
   - Retarget to `net10.0`
   - Keep `Avalonia` + `Avalonia.Desktop`
   - Add analyzer discipline (`EnableNETAnalyzers`, `AnalysisLevel`)

2. **`samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs`**
   - Keep as the Win32 seam
   - Make Windows-only surface analyzer-visible

3. **`samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`**
   - Add an explicit non-Windows warning/no-launch path

4. **`samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs`**
   - Keep Windows-only in behavior
   - Do not broaden runtime lookup to non-Windows payloads

5. **`samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj`**
   - Best candidate for the plain `net10.0` compatibility proof host

6. **`samples/avalonia-host/AvaloniaHost.csproj`**
   - Keep Windows-focused if it remains the real attach/diagnostic harness

7. **`samples/avalonia-mdcad-control/QUICKSTART.md`**
   - Split compile-time host compatibility from runtime viewer support

## What not to add

- Do **not** keep the control Windows-only at the public TFM level
- Do **not** rely on RID-only changes as the compatibility fix
- Do **not** add WPF/WinForms/WindowsDesktop SDK settings
- Do **not** add non-Windows mdCAD runtime bundles
- Do **not** introduce IPC or in-process embedding for this milestone

## Bottom line

Retarget `MdCad.Avalonia.Control` to **`net10.0`**, keep Avalonia dependencies as-is, add platform annotations/analyzers, and make non-Windows behavior explicitly warning-only.

This widens **host compile-time compatibility** without widening **runtime embedding support**.
