# Phase 44: Embedded Resize, Focus & Viewer Layout - Context

**Gathered:** 2026-05-14
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 44 hardens the hosted mdCAD interaction contract after the Phase 43 child-window bootstrap. This phase covers embedded resize correctness, focus and keyboard handoff, mouse-capture/drag behavior, viewer-first embedded layout policy, and clean shutdown when the host lifecycle ends.

In scope:
- Reliable resize behavior for the embedded mdCAD surface inside the host region
- Focus and keyboard ownership rules between the Avalonia host and mdCAD
- Mouse-capture and drag lifecycle behavior for camera/gizmo interactions
- Embedded dock/layout policy for the already-approved panel set
- Clean shutdown/orphan behavior when the host closes or the parent HWND becomes invalid

Out of scope:
- Launch-time JSONL auto-import and startup import error handling (Phase 45)
- Launch-time live refresh flags and observer arming (Phase 46)
- Bundled example JSONL resolution and milestone-complete sample-host status UX (Phase 47)
- Rich host/global shortcuts, IPC, multi-viewer hosting, or cross-platform embedding

</domain>

<decisions>
## Implementation Decisions

### Focus handoff and keyboard ownership
- **D-01:** mdCAD takes full keyboard ownership only after the user clicks inside the embedded surface.
- **D-02:** Once focused, mdCAD keeps keyboard ownership until the user clicks another host control or host window area.
- **D-03:** `Tab` remains an mdCAD shortcut; it does not move focus back to the host UI.
- **D-04:** Embedded attach must not auto-focus mdCAD at launch; focus waits for the first deliberate user click.

### Drag and capture behavior
- **D-05:** Active camera/gizmo drags continue until mouse-up while the host app stays active, even if the pointer leaves the embedded surface or host-window bounds.
- **D-06:** If the host application deactivates, mdCAD cancels the active interaction immediately and clears capture state rather than resuming later.

### Embedded panel layout
- **D-07:** The embedded default dock layout keeps the 3D viewport in the center, docks Scene Hierarchy below it, and stacks Entity Inspector, Controls, Visibility, and Camera Debug to the right of Scene Hierarchy within the same bottom dock region.
- **D-08:** Embedded mode remembers user layout adjustments, but persists them separately from standalone mdCAD layout state.
- **D-09:** The visible embedded panel set carries forward from Phase 43: Scene Hierarchy, Entity Inspector, Controls, Visibility, and Camera Debug remain available; Pick Buffer Debug, Slot Buffer Debug, and FPS Debug stay off by default.

### Shutdown and orphan behavior
- **D-10:** If the host closes or the parent HWND becomes invalid, embedded mdCAD exits immediately with no standalone fallback and no save prompt.

### the agent's Discretion
- Choose the exact Win32/Avalonia hooks used to detect surface activation, host deactivation, and focus return.
- Choose the exact cancellation mechanism for active camera/gizmo drags, as long as capture is cleared immediately on host-app deactivation.
- Choose the exact dock-split proportions and the persistence mechanism for a separate embedded layout store, as long as it remains distinct from standalone layout state.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 44 goal, success criteria, and dependency chain into Phases 45-47.
- `.planning/REQUIREMENTS.md` — `EMBD-04` and `INPT-01..04` definitions plus the remaining v1.8 milestone scope.
- `.planning/PROJECT.md` — v1.8 scope, no-IPC boundary, and the carried-forward product decisions from Phase 43.
- `.planning/STATE.md` — current milestone position after Phase 43 completion.

### Upstream embedded baseline
- `.planning/phases/43-embed-contract-child-window-bootstrap/43-CONTEXT.md` — bootstrap contract, staged attach states, and original Phase 43 boundary.
- `.planning/phases/43-embed-contract-child-window-bootstrap/43-VERIFICATION.md` — verified child-window bootstrap behavior and the currently proven host/embed baseline.
- `.planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md` — PASS attach/failure evidence that Phase 44 must preserve while improving interaction behavior.

### Code anchors
- `samples/avalonia-host/MainWindow.axaml.cs` — current `NativeControlHost` placeholder, auto-launch, attach detection, resize sync timer, and close cleanup hooks.
- `samples/avalonia-host/MainWindow.axaml` — host UI structure and embed-surface region.
- `src/app.c` — Sokol init/frame/event flow, embedded defaults, global dockspace, and keyboard shortcut processing.
- `src/ui/ui_viewport.h` — viewport hover/click and orbit-camera input gating logic.
- `src/orbit_camera.h` — current drag lifecycle and mouse-driven camera control behavior.
- `src/imgui_storage.h` — current ImGui layout persistence path that Phase 44 can extend for embedded-vs-standalone layout separation.
- `src/platform/win32_embed.h` — embedded-mode state handoff and fail-fast startup helpers carried forward from Phase 43.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `samples/avalonia-host/MainWindow.axaml.cs`: already owns the placeholder HWND, detects child attach, synchronizes child bounds during resize, and tears down the mdCAD process on host close.
- `src/app.c`: already centralizes embedded startup state, debug-window defaults, the frame loop, and all global keyboard shortcut handling.
- `src/ui/ui_viewport.h`: already computes viewport hover/click state and gates orbit-camera input through ImGui IO.
- `src/imgui_storage.h`: already persists ImGui layout state and is the natural seam for separate embedded layout persistence.

### Established Patterns
- Input currently flows through `event(const sapp_event* ev)` into `simgui_handle_event(ev)` and then through frame-time `ImGuiIO`, so embedded focus policy should plug into the existing event flow rather than inventing a parallel input stack.
- Viewer UI is driven by a global dockspace plus persisted ImGui layout, not by hardcoded per-mode panel geometry.
- The sample host owns the child HWND lifecycle in Avalonia/Win32 code, while mdCAD owns the render/input lifecycle inside that child window.
- Embedded-mode defaults are centralized in `init()` via `state.launch.embedded`, which is the right place to keep mode-specific UI/input policy coherent.

### Integration Points
- Avalonia placeholder/child HWND handling in `MainWindow.axaml.cs` for click-to-focus, host deactivation detection, and parent-invalid shutdown.
- `event(const sapp_event* ev)` plus the frame loop in `src/app.c` for focus-loss handling and capture cancellation.
- `ui_viewport_draw()` plus orbit camera/gizmo drag paths for continuing or canceling interactions when hover/focus changes.
- ImGui settings persistence (`imgui.ini` seam) for a separate embedded layout namespace/file.
- Existing host close/child process cleanup path for immediate embedded exit with no fallback window.

</code_context>

<specifics>
## Specific Ideas

- Full keyboard ownership begins only after the user deliberately clicks inside the embedded viewer.
- `Tab` stays with mdCAD; clicking host UI is the way to return focus to the host.
- Active drags may continue off-surface while the host app stays active, but must cancel immediately on host-app deactivation.
- The desired embedded dock arrangement is: viewport centered, Scene Hierarchy below, and Entity Inspector + Controls + Visibility + Camera Debug stacked to the right of Scene Hierarchy within the same bottom dock.
- Embedded layout adjustments should persist separately from standalone mdCAD layout state.

</specifics>

<deferred>
## Deferred Ideas

- Launch-time JSONL flags and startup import error UX remain Phase 45 work.
- Launch-time live refresh and observer opt-in remain Phase 46 work.
- Bundled example JSONL resolution plus richer JSONL/live-refresh host status remain Phase 47 work.
- Rich host/global shortcuts or explicit host-side focus-return controls are not required for Phase 44.
- Embedded chrome trimming/polish beyond the current approved panel set remains later polish, not a Phase 44 blocker.

</deferred>

---

*Phase: 44-embedded-resize-focus-viewer-layout*
*Context gathered: 2026-05-14*
