# Project Research Summary

**Project:** mdCAD v1.9 — Plain `net10.0` Avalonia Host Compatibility  
**Domain:** host-facing TFM compatibility widening for a Windows-only embedded mdCAD control  
**Researched:** 2026-05-18  
**Confidence:** HIGH

## Executive Summary

v1.9 should widen the **host compile-time contract** for the reusable control without widening the **runtime embedding contract** for mdCAD itself. The control should become referenceable from a plain `net10.0` Avalonia host, while the actual embedded viewer remains Windows-only and renders a safe placeholder/warning on unsupported platforms.

The recommended implementation path is to retarget the public control project to `net10.0`, keep the existing Avalonia package line, and split the control internals into a shared shell plus explicit Windows and unsupported-platform backends. That preserves the proven Windows child-HWND/runtime bundle path while making non-Windows behavior intentional instead of accidental.

The main risk is not “can Avalonia compile cross-platform”; it is **blurring the runtime boundary** so consumers misread compile-time compatibility as cross-platform embedding support. The milestone will fail if it merely changes TFMs, leaves Win32 semantics smeared across shared code, or regresses the already-shipped Windows harness path.

## Key Findings

### Stack additions

- Public control should target **`net10.0`**
- Keep **`Avalonia` + `Avalonia.Desktop`** as-is
- Add stronger platform guardrails (`OperatingSystem.IsWindows()`, CA1416 discipline, platform annotations on Windows-only internals)
- Keep the runtime bundle explicitly Windows-only (`mdcad-runtime\win-x64`)

### Feature table stakes

- Plain `net10.0` host can reference and build against the control without `NU1201`
- Non-Windows runtime shows a visible placeholder/warning and never attempts mdCAD launch
- Windows runtime behavior stays unchanged: packaging, attach, JSONL startup, live refresh opt-in, sealed/diagnostic modes
- Public docs explain compile-time compatibility separately from runtime viewer support
- Imperative APIs (`AutoStart`, `StartAsync`, `StopAsync`) have explicit unsupported-platform behavior

### Recommended architecture

- Keep one public assembly: `MdCad.Avalonia.Control`
- Split internals into:
  - shared `MdCadEmbeddedControl` shell
  - `WindowsMdCadEmbedBackend`
  - `UnsupportedMdCadEmbedBackend`
- Move placeholder HWND, attach, resize sync, runtime lookup, and process launch behind the Windows backend
- Keep `samples/avalonia-host` as the authoritative Windows diagnostic harness
- Use `samples/avalonia-host-minimal` as the plain `net10.0` compatibility proof

### Watch out for

- Fixing only sample hosts while leaving the control itself Windows-only
- Leaving non-Windows behavior implicit/blank
- Breaking the shipped Windows runtime-copy or attach/relaunch path
- Letting Windows-only implementation leak into the public surface
- Updating docs/tests incompletely so the new host contract is not actually proven

## Roadmap implications

The roadmap should absorb risk in this order:

1. Extract the backend seam while preserving current Windows behavior
2. Add explicit unsupported-platform backend/placeholder behavior
3. Retarget the public control project to `net10.0`
4. Add plain-`net10.0` consumer proof while preserving the Windows harness
5. Close docs/tests/packaging wording around the compile-time vs runtime contract

## Bottom line

The best-fit milestone is **not** “cross-platform mdCAD embedding.”  
It is **plain `net10.0` host compatibility for a still-Windows-only embedded viewer runtime**.
