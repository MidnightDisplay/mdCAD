# Requirements: mdCAD

**Defined:** 2026-05-14
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1 Requirements

### Embedding Contract

- [x] **EMBD-01**: Developer can launch mdCAD in Windows embedded mode by passing a parent HWND on the command line. — Validated in Phase 43
- [x] **EMBD-02**: Embedded mdCAD creates and remains inside a true child window under the supplied parent HWND instead of silently falling back to a standalone top-level window. — Validated in Phase 43
- [x] **EMBD-03**: Developer gets a clear startup failure when embedded-mode arguments are missing, invalid, or incompatible. — Validated in Phase 43
- [x] **EMBD-04**: Embedded mdCAD shuts down cleanly when the host/control lifecycle ends or the parent HWND becomes invalid.

### Embedded Interaction

- [x] **INPT-01**: User can resize the host control and mdCAD resizes its hosted render surface without clipped, stale, or incorrect viewport behavior.
- [x] **INPT-02**: User can click into the embedded viewer and immediately use mdCAD keyboard and mouse interactions without host interference.
- [x] **INPT-03**: User can move focus between host UI and mdCAD without stuck capture, stuck drag, or broken input state.
- [x] **INPT-04**: User sees mdCAD in a viewer-first embedded layout that fits the hosted region.

### Startup JSONL

- [x] **JSON-01**: Developer can pass an absolute JSONL path and mdCAD auto-imports it at startup using the large flat dump workflow.
- [x] **JSON-02**: Developer can opt into live refresh for the startup JSONL import with an explicit command-line flag.
- [x] **JSON-03**: User keeps a usable embedded viewer and gets a clear error state when startup JSONL import fails.
- [x] **JSON-04**: Launch-time live refresh reuses the existing linked flat JSONL observer semantics without changing default refresh behavior for other workflows.

### Sample Host

- [x] **HOST-01**: Developer can build and run a minimal Avalonia sample that embeds mdCAD inside a `NativeControlHost`. — Validated in Phase 43
- [x] **HOST-02**: Sample host resolves a bundled example JSONL from a relative `resources/examples` folder and launches mdCAD with its absolute path. — Validated in Phase 47
- [x] **HOST-03**: Sample host shows launch, attach, JSONL, and live-refresh status text for the embedded session. — Validated in Phase 47
- [x] **HOST-04**: Sample host can repeatedly launch, resize, focus, and close the embedded mdCAD session without leaving orphaned processes. — Validated in Phase 47

### Reusable Avalonia Control

- [x] **P48-01**: External Windows Avalonia apps can reference a reusable Windows-only mdCAD control library instead of duplicating the sample host's window-owned embedding seam. — Validated in Phase 48 Plan 01
- [x] **P48-02**: Building a consuming app copies a pinned `mdcad-runtime/` bundle into consumer output and the control resolves mdCAD from that copied runtime only.
- [x] **P48-03**: A XAML-dropped control auto-starts by default once both bindings and the placeholder HWND are ready.
- [x] **P48-04**: Host apps can opt out with `AutoStart=false` and explicitly start and stop the embedded session without orphaning mdCAD.
- [x] **P48-05**: Changing launch-affecting properties while mdCAD is running produces one serialized relaunch using the newest requested snapshot only.
- [x] **P48-06**: `sealed` mode stays bare by default, `diagnostic` mode is explicit opt-in, and a bad requested JSONL path stays visibly warned without blocking a usable viewer launch.
- [x] **P48-07**: The reusable control preserves the existing no-IPC, separate-process, Windows-only embedding contract and prior embedded regression coverage.

## v2 Requirements

### Host Integration Extensions

- **HOSTX-01**: Host can send structured control commands to a running embedded mdCAD session without relaunching the process.
- **HOSTX-02**: Host can receive machine-readable readiness, import, and refresh status events from mdCAD.
- **HOSTX-03**: Host can embed and manage multiple mdCAD child viewers in one application session.

### Embedded UX Polish

- **EUX-01**: User can view mdCAD in a more purpose-built embedded chrome mode with standalone-only UI trimmed away.
- **EUX-02**: User can control embedded JSONL reload and refresh behavior through richer sample-host controls.

## Out of Scope

| Feature | Reason |
|---------|--------|
| In-process / DLL / SDK embedding | This milestone keeps mdCAD as a separate launched process to minimize architecture and lifetime risk |
| Rich host-to-viewer IPC | The first milestone stays CLI-driven and avoids expanding the integration contract before child-window hosting is stable |
| Cross-platform embedding parity | v1.8 is intentionally scoped to the Windows + Avalonia host workflow first |
| Renderer rewrites or backend swaps | The new work is a Win32 windowing/lifecycle problem, not a rendering architecture change |
| Refresh architecture redesign | v1.8 should reuse the existing linked flat JSONL import/observer semantics rather than reopening settled refresh behavior |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| EMBD-01 | Phase 43 | Complete |
| EMBD-02 | Phase 43 | Complete |
| EMBD-03 | Phase 43 | Complete |
| EMBD-04 | Phase 44 | Complete |
| INPT-01 | Phase 44 | Complete |
| INPT-02 | Phase 44 | Complete |
| INPT-03 | Phase 44 | Complete |
| INPT-04 | Phase 44 | Complete |
| JSON-01 | Phase 45 | Complete |
| JSON-02 | Phase 46 | Complete |
| JSON-03 | Phase 45 | Complete |
| JSON-04 | Phase 46 | Complete |
| HOST-01 | Phase 43 | Complete |
| HOST-02 | Phase 47 | Complete |
| HOST-03 | Phase 47 | Complete |
| HOST-04 | Phase 47 | Complete |
| P48-01 | Phase 48 | Complete |
| P48-02 | Phase 48 | Complete |
| P48-03 | Phase 48 | Complete |
| P48-04 | Phase 48 | Complete |
| P48-05 | Phase 48 | Complete |
| P48-06 | Phase 48 | Complete |
| P48-07 | Phase 48 | Complete |

**Coverage:**
- v1 requirements: 23 total
- Mapped to phases: 23
- Unmapped: 0 ✓

---
*Requirements defined: 2026-05-14*
*Last updated: 2026-05-15 after Phase 48 Plan 01*
