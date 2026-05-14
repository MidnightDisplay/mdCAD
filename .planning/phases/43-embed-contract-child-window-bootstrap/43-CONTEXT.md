# Phase 43: Embed Contract & Child-Window Bootstrap - Context

**Gathered:** 2026-05-14
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 43 delivers the strict bootstrap contract for launching mdCAD as an embedded Windows child window from a host application.

In scope:
- Embedded launch contract for `--embedded` + `--parent-hwnd`
- True child-window startup behavior with no silent standalone fallback
- Clear startup failure behavior for invalid or incompatible embed args
- Minimal Avalonia sample host that can build, run, and embed mdCAD

Out of scope:
- Embedded resize/focus/input correctness beyond bootstrap viability (Phase 44)
- Launch-time JSONL auto-import (Phase 45)
- Launch-time live refresh wiring (Phase 46)
- Bundled example JSONL and full end-to-end workflow proof (Phase 47)
- Any IPC, in-process embedding, or cross-platform host support

</domain>

<decisions>
## Implementation Decisions

### Launch contract
- **D-01:** Phase 43 uses a strict embedded launch contract based on `--embedded` and `--parent-hwnd`.
- **D-02:** If embedded arguments are invalid or child-window startup fails, mdCAD must fail fast and exit with a clear error.
- **D-03:** Embedded mode must never silently fall back to a standalone top-level mdCAD window.

### Sample bootstrap flow
- **D-04:** The minimal Avalonia sample should auto-launch mdCAD as soon as the host control is ready.
- **D-05:** The sample should surface staged bootstrap states: `launching`, `waiting for child attach`, `attached`, and `timeout/failure`.

### Embedded visual baseline
- **D-06:** Standalone chrome should be trimmed immediately in Phase 43 instead of waiting until Phase 44.
- **D-07:** Embedded-mode defaults should turn OFF the existing debug-window visibility toggles for `Pick Buffer Debug`, `Slot Buffer Debug`, and `FPS Debug`.

### the agent's Discretion
- Accept both `0x`-prefixed hex and decimal parent-HWND values if that does not change the visible launch contract.
- Choose the exact attach-timeout duration and polling cadence, provided the staged bootstrap states remain visible and failures stay explicit.
- Decide the exact trimmed-chrome surface for Phase 43, as long as obvious standalone-heavy UI is reduced immediately and planning for Phase 44 can still refine the viewer-first layout.

</decisions>

<specifics>
## Specific Ideas

- The intended host shape is an Avalonia Windows desktop app using `NativeControlHost` with a Win32 child HWND.
- The user-provided launch example is `--embedded --parent-hwnd 0x12345678`.
- The host should stay intentionally minimal: a control for the embedded surface plus status messaging, not a rich management UI.
- The user explicitly wants the embedded bootstrap to default `Pick Buffer Debug`, `Slot Buffer Debug`, and `FPS Debug` to OFF via the existing Visibility-window debug controls.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 43 goal, dependencies, and success criteria.
- `.planning/REQUIREMENTS.md` — `EMBD-01`, `EMBD-02`, `EMBD-03`, and `HOST-01` definitions.
- `.planning/PROJECT.md` — v1.8 milestone scope, Windows-only boundary, and no-IPC constraint.
- `.planning/STATE.md` — active milestone position and continuity.

### Phase research
- `.planning/research/SUMMARY.md` — consolidated phase-order rationale, risks, and non-goals.
- `.planning/research/STACK.md` — Windows child-HWND + Avalonia host stack recommendation.
- `.planning/research/ARCHITECTURE.md` — launch-config, embed adapter, and Sokol seam recommendations.
- `.planning/research/PITFALLS.md` — `SetParent` risks, readiness races, and bootstrap lifecycle hazards.

### Upstream phase behavior to preserve
- `.planning/phases/36-flat-import-entry-configuration/36-CONTEXT.md` — flat JSONL entry path and default-OFF observer opt-in contract that later phases must preserve.
- `.planning/phases/41-linked-import-convergence/41-CONTEXT.md` — linked observer baseline and no-self-refresh semantics for later launch-time JSONL phases.

### Code anchors
- `src/app.c` — current `sokol_main(argc, argv)` entry point, full-window frame sizing, and central UI/render tick.
- `src/platform.h` — current Windows backend selection rules.
- `src/ui/ui_visibility.h` — existing `Debug Windows` visibility controls used to default the requested debug windows OFF in embedded mode.
- `src/ui/ui_scene_hierarchy.h` — existing flat JSONL import action and observer-contract capture for later JSONL launch phases.
- `src/jsonl_import_job.h` — existing flat import job and observer-contract helpers for later reuse.
- `vendors/libsokol/CMakeLists.txt` — current Sokol fetch/pinning setup and Windows backend linkage.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/app.c`: `sokol_main()` already centralizes startup and currently ignores `argc/argv`, making it the natural place to parse embedded-launch flags.
- `src/ui/ui_visibility.h`: already exposes the `Debug Windows` toggles for `Pick Buffer Debug`, `Slot Buffer Debug`, and `FPS Debug`.
- `src/ui/ui_scene_hierarchy.h`: already contains the flat JSONL import entry point and observer opt-in contract that later startup-import phases should reuse.
- `src/jsonl_import_job.h`: already supports observer-contract capture and linked-baseline behavior for later phases.

### Established Patterns
- Startup behavior is centralized in `sokol_main()` + `app.c`, not scattered across multiple UI modules.
- UI/debug windows are drawn from `app.c` each frame when `state.ui_visible` is enabled, so embedded-mode visibility defaults can be controlled centrally.
- Flat JSONL import flows already use chunked jobs and optional observer contracts rather than ad hoc startup logic.
- Windows backend selection is compile-time/platform-driven through `src/platform.h` and CMake rather than per-feature runtime switching.

### Integration Points
- `src/app.c` for launch-config parsing, embedded bootstrap mode selection, and bootstrap status flow.
- The Win32/Sokol window-creation seam for true child-window startup rather than post-create reparenting.
- `src/ui/ui_visibility.h` plus whatever initializes visibility state for embedded-mode default debug-window OFF behavior.
- A new sample-host directory outside core runtime code so Avalonia/.NET stays separate from mdCAD's native build.

</code_context>

<deferred>
## Deferred Ideas

- Startup JSONL launch flags (`--jsonl`, `--jsonl-live-refresh`) remain Phase 45 and Phase 46 work.
- Full embedded resize/focus/input correctness remains Phase 44, even though obvious standalone chrome trimming is pulled into Phase 43.
- Bundled example JSONL resolution and full workflow status coverage remain Phase 47 work.
- Rich host controls, richer IPC/status channels, multiple embedded instances, and cross-platform hosting remain out of scope for this phase.

</deferred>

---

*Phase: 43-embed-contract-child-window-bootstrap*
*Context gathered: 2026-05-14*
