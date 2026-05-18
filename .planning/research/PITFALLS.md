# Domain Pitfalls

**Domain:** Plain `net10.0` Avalonia host compatibility for a Windows-only embedded mdCAD control  
**Researched:** 2026-05-18  
**Confidence:** HIGH

## Critical pitfalls

### 1. Fixing only the samples instead of the control asset

If `MdCad.Avalonia.Control` stays `net10.0-windows...`, external plain `net10.0` consumers will still fail with `NU1201` even if repo samples look fixed.

**Prevention:** ship a plain `net10.0` consumable asset for the control itself.

### 2. Letting Windows-only implementation leak into the public surface

If the public API effectively stays Windows-only, consumers will hit CA1416 noise, XAML load friction, or platform-specific assumptions despite the wider TFM.

**Prevention:** keep Win32 types/helpers internal; keep the public control API platform-neutral.

### 3. Leaving non-Windows behavior implicit

Relying on “placeholder handle never appears” creates a silent idle state instead of a real unsupported-platform contract.

**Prevention:** add an explicit unsupported-platform warning/placeholder and a deterministic no-launch path.

### 4. Breaking the already-shipped Windows harness path

The compatibility widening must not regress:
- attach
- relaunch
- runtime bundle lookup
- close/teardown ordering

**Prevention:** treat the current Windows HWND/runtime path as locked; widen around it, not through it.

### 5. Breaking runtime packaging while chasing cross-platform builds

The current `mdcad-runtime\mdCAD.exe` copy flow can be accidentally broken by TFM conditions or packaging tweaks.

**Prevention:** preserve the exact `AppContext.BaseDirectory\\mdcad-runtime` contract unless there is a deliberate packaging redesign.

## Moderate pitfalls

### 6. Misleading non-Windows users with Windows-only JSONL validation

If non-Windows hosts see a “path must be absolute” launch warning first, the contract becomes confusing because runtime support is the real issue.

**Prevention:** unsupported-platform warning takes precedence over launch/path diagnostics.

### 7. `StartAsync()` remaining callable but semantically undefined

Hosts need a clear imperative contract on unsupported platforms.

**Prevention:** define `StartAsync()` as immediate, explicit unsupported-runtime failure or stable no-op with warning; keep `StopAsync()` safe when nothing can run.

### 8. Multi-target drift

If both `net10.0` and `net10.0-windows...` assets exist, they can drift in API or behavior.

**Prevention:** keep one shared public API and isolate only Windows internals.

## Minor pitfalls

### 9. Docs staying in the old Windows-only-host shape

If `QUICKSTART.md` still says “target `net10.0-windows...`”, consumers will never discover the widened host contract.

**Prevention:** document compile-time host compatibility separately from runtime viewer compatibility.

### 10. Tests staying Windows-targeted only

Repo tests can all pass while the new plain-`net10.0` contract remains unproven.

**Prevention:** add at least one plain-`net10.0` consumer smoke and one unsupported-platform behavior seam.

## Recommended rollout order

1. **Compile-time compatibility first**
   - Make the control referenceable from plain `net10.0`
   - Add a plain `net10.0` consumer restore/build proof

2. **Unsupported-platform contract second**
   - Add explicit no-launch placeholder behavior
   - Ensure unsupported platforms never attempt mdCAD launch

3. **Windows regression closure third**
   - Re-run the shipped harness path checks
   - Verify runtime copy, attach, stop, relaunch

4. **Docs/tests cleanup fourth**
   - Rewrite quickstart around compile-time vs runtime contract
   - Add consumer-facing proof that the widened host contract is real
