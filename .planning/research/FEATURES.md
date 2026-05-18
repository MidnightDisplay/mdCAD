# Feature Landscape

**Project:** mdCAD v1.9 — Plain `net10.0` Avalonia Host Compatibility  
**Scope:** host-facing compatibility widening only; no embedding redesign  
**Researched:** 2026-05-18  
**Confidence:** HIGH for table stakes, MEDIUM for differentiators

## Scope Anchor

This milestone should make the **host project** portable at compile time while keeping the **embedded mdCAD viewer** Windows-only at runtime.

That means:

- A plain `net10.0` Avalonia host should be able to reference the control.
- The viewer should still launch only through the existing Windows child-HWND path.
- Non-Windows runtime behavior must be explicit, safe, and unsurprising.

## Table Stakes

| Feature | Why expected | Requirement for this milestone |
|---------|--------------|--------------------------------|
| Plain `net10.0` host reference succeeds | This is the blocker that created v1.9 | A plain `net10.0` Avalonia host must restore/build/reference the control without `NU1201` |
| XAML-safe control on cross-platform hosts | Consumers will drop the control into shared Avalonia views | The control must instantiate and render on non-Windows without type-load/PInvoke crashes |
| Windows runtime behavior unchanged | Compile-time widening must not regress the shipped viewer path | On Windows, keep current runtime packaging, `mdcad-runtime` lookup, child-HWND attach, JSONL startup, live-refresh opt-in, and sealed/diagnostic modes |
| Explicit unsupported-platform placeholder | A blank surface looks broken and implies a bug | On non-Windows, render a visible in-control message that the host is compatible but embedded mdCAD viewing is Windows-only |
| No launch attempt on unsupported platforms | Silent retries or hidden failures would confuse hosts | `AutoStart` must not try to launch mdCAD on unsupported platforms; the control should stay in placeholder mode |
| Deterministic imperative API behavior | Programmatic hosts need a clear contract too | `StartAsync()` on unsupported platforms must fail clearly and immediately; `StopAsync()` must remain safe/no-op when nothing can run |
| Diagnostic mode tells the truth | Diagnostic chrome is the proof surface | Diagnostic mode should show `unsupported platform` / `viewer unavailable` state and disable launch actions when runtime activation is impossible |
| Clear Windows-only capability wording | Compile-time compatibility is not runtime parity | Public docs and sample behavior must state that plain `net10.0` compatibility does **not** mean mdCAD embeds on macOS/Linux |
| Explicit Windows failure messaging | Windows hosts still need actionable failures | Missing runtime bundle, bad executable, or attach failure should surface as host-owned warning/detail text, not as silent non-start |

## Desirable Differentiators (only if low-risk)

| Feature | Value | Why it is optional |
|---------|-------|--------------------|
| `IsRuntimeSupported` / similar read-only capability property | Lets hosts hide their own launch affordances without parsing warning text | Nice API polish, but not required if the control already renders truthful placeholder state |
| Custom unsupported-platform placeholder text | Helps app teams match product tone while preserving the contract | Useful but easy to defer if it complicates the sealed surface |
| Minimal plain-`net10.0` sample host | Proves the exact new consumer scenario end-to-end | Valuable documentation artifact, but only if it reuses the current minimal sample instead of spawning another harness |
| Analyzer-friendly platform annotations on Windows-only internals/APIs | Helps keep future compatibility changes honest | Good maintenance guardrail, but implementation detail rather than milestone-defining feature |

## Anti-Features / Scope Traps

| Anti-feature | Why avoid it | What to do instead |
|--------------|--------------|--------------------|
| Cross-platform embedding promise | The runtime seam is still Win32 child-HWND | Keep compile-time compatibility broad, runtime viewer support Windows-only |
| New host↔viewer IPC surface | Expands the milestone into control-protocol design | Keep the existing process launch + CLI contract |
| Standalone-window fallback on unsupported platforms | Hides contract violations and creates misleading behavior | Show placeholder/warning and do nothing |
| Packaging redesign beyond current Windows bundle | Not needed to solve host compatibility | Preserve current `mdcad-runtime` contract on Windows |
| Broader control API expansion | Risks turning a compatibility fix into a product-surface redesign | Limit additions to capability/placeholder clarity only |
| Multi-instance/session semantics work | Separate lifetime problem | Preserve current single-control/session behavior |
| “Cross-platform viewer” messaging in docs or samples | Would overstate what shipped | Be explicit: cross-platform host compile, Windows-only viewer runtime |

## Consumer-Contract Requirements

These should become explicit requirements, not implied behavior:

1. **Compile-time vs runtime matrix**
   - Document separately:
     - `net10.0` host can reference the control
     - Windows runtime can launch embedded mdCAD
     - non-Windows runtime shows placeholder only

2. **Behavior contract for supported vs unsupported platforms**
   - Windows: existing embed flow works as today
   - Non-Windows: no launch attempt, no crash, visible warning, predictable API behavior

3. **`AutoStart` / `StartAsync` / `StopAsync` expectations**
   - `AutoStart`: launch on supported Windows only
   - `StartAsync`: explicit immediate failure on unsupported runtime
   - `StopAsync`: safe when no session is active

4. **Minimal onboarding docs must change**
   - Quickstart can no longer say “target `net10.0-windows...`” as the only path
   - It must explain that plain `net10.0` is valid for the host, while the embedded viewer remains Windows-only

5. **Host-experience gap to avoid**
   - Do not leave unsupported platforms as a silent empty region or permanently disabled control with no explanation

## MVP Recommendation

Prioritize:

1. Plain `net10.0` host reference/build success
2. Explicit unsupported-platform placeholder contract
3. Preserve current Windows runtime behavior unchanged
4. Update quickstart/README/sample wording so consumers cannot misread the capability boundary

Defer:

- Cosmetic placeholder customization
- Any new host-control protocol
- Any attempt at non-Windows embedding

## Sources

- `C:\dev\mdCAD\.planning\PROJECT.md`
- `C:\dev\mdCAD\.planning\STATE.md`
- `C:\dev\MDCAD\README.md`
- `C:\dev\mdCAD\samples\avalonia-mdcad-control\QUICKSTART.md`
- Current control implementation:
  - `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj`
  - `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs`
  - `samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs`
  - `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs`
- Microsoft .NET target frameworks (official): https://learn.microsoft.com/en-us/dotnet/standard/frameworks
- Microsoft platform compatibility analyzer (official): https://learn.microsoft.com/en-us/dotnet/standard/analyzers/platform-compat-analyzer
