# Requirements: mdCAD

**Defined:** 2026-05-14
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1 Requirements

### Embedding Contract

- [ ] **EMBD-01**: Developer can launch mdCAD in Windows embedded mode by passing a parent HWND on the command line.
- [ ] **EMBD-02**: Embedded mdCAD creates and remains inside a true child window under the supplied parent HWND instead of silently falling back to a standalone top-level window.
- [ ] **EMBD-03**: Developer gets a clear startup failure when embedded-mode arguments are missing, invalid, or incompatible.
- [ ] **EMBD-04**: Embedded mdCAD shuts down cleanly when the host/control lifecycle ends or the parent HWND becomes invalid.

### Embedded Interaction

- [ ] **INPT-01**: User can resize the host control and mdCAD resizes its hosted render surface without clipped, stale, or incorrect viewport behavior.
- [ ] **INPT-02**: User can click into the embedded viewer and immediately use mdCAD keyboard and mouse interactions without host interference.
- [ ] **INPT-03**: User can move focus between host UI and mdCAD without stuck capture, stuck drag, or broken input state.
- [ ] **INPT-04**: User sees mdCAD in a viewer-first embedded layout that fits the hosted region.

### Startup JSONL

- [ ] **JSON-01**: Developer can pass an absolute JSONL path and mdCAD auto-imports it at startup using the large flat dump workflow.
- [ ] **JSON-02**: Developer can opt into live refresh for the startup JSONL import with an explicit command-line flag.
- [ ] **JSON-03**: User keeps a usable embedded viewer and gets a clear error state when startup JSONL import fails.
- [ ] **JSON-04**: Launch-time live refresh reuses the existing linked flat JSONL observer semantics without changing default refresh behavior for other workflows.

### Sample Host

- [ ] **HOST-01**: Developer can build and run a minimal Avalonia sample that embeds mdCAD inside a `NativeControlHost`.
- [ ] **HOST-02**: Sample host resolves a bundled example JSONL from a relative `resources/examples` folder and launches mdCAD with its absolute path.
- [ ] **HOST-03**: Sample host shows launch, attach, JSONL, and live-refresh status text for the embedded session.
- [ ] **HOST-04**: Sample host can repeatedly launch, resize, focus, and close the embedded mdCAD session without leaving orphaned processes.

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
| EMBD-01 | — | Pending |
| EMBD-02 | — | Pending |
| EMBD-03 | — | Pending |
| EMBD-04 | — | Pending |
| INPT-01 | — | Pending |
| INPT-02 | — | Pending |
| INPT-03 | — | Pending |
| INPT-04 | — | Pending |
| JSON-01 | — | Pending |
| JSON-02 | — | Pending |
| JSON-03 | — | Pending |
| JSON-04 | — | Pending |
| HOST-01 | — | Pending |
| HOST-02 | — | Pending |
| HOST-03 | — | Pending |
| HOST-04 | — | Pending |

**Coverage:**
- v1 requirements: 16 total
- Mapped to phases: 0
- Unmapped: 16 ⚠

---
*Requirements defined: 2026-05-14*
*Last updated: 2026-05-14 after initial definition*
