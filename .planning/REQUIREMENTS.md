# Requirements: mdCAD

**Defined:** 2026-05-15
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1 Requirements

### Host Compatibility

- [ ] **HOSTC-01**: Developer can reference `MdCad.Avalonia.Control` from a plain `net10.0` Avalonia host without `NU1201` or equivalent restore/build compatibility failures.
- [ ] **HOSTC-02**: Developer can instantiate `MdCadEmbeddedControl` from shared XAML/code in a plain `net10.0` Avalonia host without platform-specific type-load or startup crashes.

### Unsupported Platform Contract

- [ ] **PLAT-01**: User sees a clear placeholder/warning on non-Windows instead of a blank or broken embedded surface.
- [ ] **PLAT-02**: On non-Windows, `AutoStart` and launch-affecting property changes never attempt to start mdCAD.
- [ ] **PLAT-03**: On non-Windows, `StartAsync()` fails clearly with an unsupported-runtime result and `StopAsync()` remains safe when no session can exist.

### Windows Runtime Preservation

- [ ] **WPRS-01**: On Windows, the control still resolves `mdcad-runtime\mdCAD.exe` from consumer output and launches mdCAD with the existing child-HWND contract.
- [ ] **WPRS-02**: On Windows, sealed and diagnostic modes preserve current attach/relaunch/status behavior after the host-compatibility widening.
- [ ] **WPRS-03**: Windows consumer proof hosts still verify copied-runtime presence plus attach/stop/relaunch lifecycle without regressions.

### Consumer Proof and Docs

- [ ] **PROOF-01**: Repository includes a plain `net10.0` Avalonia consumer proof that references the control directly and builds successfully.
- [ ] **PROOF-02**: Quickstart/README clearly separate compile-time host compatibility from runtime viewer support and document unsupported-platform behavior truthfully.

## v2 Requirements

### Developer Ergonomics

- **CAP-01**: Host can query runtime support through a dedicated capability property or event instead of inferring it from warning text.
- **CAP-02**: Host can customize unsupported-platform placeholder content without retemplating the control.

### Packaging Evolution

- **PACK-01**: Control can publish a split host-facing vs Windows-implementation package layout if broader distribution needs justify it.

## Out of Scope

| Feature | Reason |
|---------|--------|
| Cross-platform mdCAD embedding runtime | v1.9 widens compile-time host compatibility only; the embedded viewer itself remains Windows-only |
| Host-to-viewer IPC or richer control protocol | Not needed to solve the current TFM/runtime-boundary problem |
| Non-Windows mdCAD runtime bundles | Would overpromise runtime support instead of clarifying the current Windows-only seam |
| Public assembly/package redesign beyond what is required for plain `net10.0` host compatibility | Adds packaging complexity without addressing the immediate blocker |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| HOSTC-01 | Phase 52 | Pending |
| HOSTC-02 | Phase 52 | Pending |
| PLAT-01 | Phase 51 | Pending |
| PLAT-02 | Phase 51 | Pending |
| PLAT-03 | Phase 51 | Pending |
| WPRS-01 | Phase 50 | Pending |
| WPRS-02 | Phase 50 | Pending |
| WPRS-03 | Phase 53 | Pending |
| PROOF-01 | Phase 53 | Pending |
| PROOF-02 | Phase 54 | Pending |

**Coverage:**
- v1 requirements: 10 total
- Mapped to phases: 10
- Unmapped: 0 ✓

---
*Requirements defined: 2026-05-15*
*Last updated: 2026-05-15 after roadmap creation*
