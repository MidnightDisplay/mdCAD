# Phase 44: Embedded Resize, Focus & Viewer Layout - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-14
**Phase:** 44-embedded-resize-focus-viewer-layout
**Areas discussed:** Focus handoff and keyboard ownership, Drag/capture behavior, Embedded panel layout, Shutdown/orphan behavior

---

## Focus handoff and keyboard ownership

### Keyboard ownership after click

| Option | Description | Selected |
|--------|-------------|----------|
| Give mdCAD full keyboard ownership until focus leaves the embedded surface | The embedded viewer owns normal keyboard input after activation. | ✓ |
| Keep a small set of host/global shortcuts active even while mdCAD is focused | Reserve some keys for the host even during viewer interaction. | |
| Require an explicit activate step before mdCAD starts handling keys | User must take a separate action before mdCAD becomes keyboard-active. | |

**User's choice:** Give mdCAD full keyboard ownership until focus leaves the embedded surface.
**Notes:** The viewer should own keys after deliberate interaction rather than sharing them with the host.

### Returning focus to the host

| Option | Description | Selected |
|--------|-------------|----------|
| Only when the user clicks another host control or window area; Tab stays with mdCAD | Host focus returns by click, and `Tab` remains available to mdCAD. | ✓ |
| Pressing Tab should leave mdCAD and move focus into the host UI | `Tab` becomes a host-focus traversal key. | |
| Provide an explicit host-side return-focus action or hotkey | Add a dedicated host affordance for leaving the viewer. | |

**User's choice:** Only when the user clicks another host control or window area; Tab stays with mdCAD.
**Notes:** `Tab` must not be repurposed away from mdCAD.

### Initial attach focus

| Option | Description | Selected |
|--------|-------------|----------|
| Wait for the first user click before mdCAD takes focus | Embedded launch does not steal focus on attach. | ✓ |
| Auto-focus mdCAD immediately after attach | Viewer becomes active as soon as it appears. | |
| Restore whichever control the host already had until the host explicitly activates mdCAD | Host keeps its existing focus until it chooses otherwise. | |

**User's choice:** Wait for the first user click before mdCAD takes focus.
**Notes:** Embedded attach should not auto-steal keyboard focus.

---

## Drag/capture behavior

### Initial drag preference

| Option | Description | Selected |
|--------|-------------|----------|
| Cancel the interaction immediately and clear any capture state | Prefer strict safety over continuing a drag across boundaries. | |
| Keep the drag alive until mouse-up if focus returns quickly | Allow some continuation, but only if focus comes back. | |
| Freeze the interaction and resume it on refocus | Suspend then restore the drag state. | |
| Other (freeform) | Custom rule described directly by the user. | ✓ |

**User's choice:** Keep the drag alive until mouse-up even if the cursor leaves the surface and even the host app window.
**Notes:** This established the desired direction for off-surface drag continuity before deactivation handling was clarified.

### Deactivation fallback

| Option | Description | Selected |
|--------|-------------|----------|
| Keep the drag alive while the host window stays active, but cancel immediately when the host app deactivates | Off-surface continuation is allowed, but app deactivation clears capture immediately. | ✓ |
| Try to preserve drag even across host-app deactivation, accepting platform-specific edge cases | Maximize continuity even across risky activation boundaries. | |
| Cancel as soon as the cursor leaves the embedded region | Prefer strict in-bounds interaction only. | |

**User's choice:** Keep the drag alive while the host window stays active, but cancel immediately when the host app deactivates.
**Notes:** This is the locked Phase 44 capture rule.

---

## Embedded panel layout

### Default dock arrangement

| Option | Description | Selected |
|--------|-------------|----------|
| Center viewport, Scene Hierarchy docked left, and Entity Inspector + Controls + Visibility + Camera Debug stacked on the right | Traditional side-panel viewer layout. | |
| Center viewport with one combined left sidebar for all supporting panels | Maximize open space by consolidating all support UI on one side. | |
| Center viewport with the supporting panels collapsed into tabs/accordion | Strongest viewport-first option with minimal simultaneous chrome. | |
| Other (freeform) | Custom dock arrangement described directly by the user. | ✓ |

**User's choice:** Viewport docked to global center, Scene Hierarchy docked below, Entity Inspector + Controls + Visibility + Camera Debug stacked to the right of the Scene Hierarchy in the same bottom dock.
**Notes:** This preserves the requested visible panel set while biasing the center of the host region toward the viewport.

### Layout persistence

| Option | Description | Selected |
|--------|-------------|----------|
| Remember user adjustments in embedded mode, but keep them separate from standalone layout | Embedded layout persists, but does not overwrite standalone mdCAD arrangement. | ✓ |
| Reset to the default embedded layout on every launch | Always return to the same default layout. | |
| Reuse the same saved layout settings as standalone mdCAD | One shared layout state across both modes. | |

**User's choice:** Remember user adjustments in embedded mode, but keep them separate from standalone layout.
**Notes:** Embedded and standalone layout persistence must not trample each other.

---

## Shutdown/orphan behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Exit immediately without any standalone fallback or save prompt | Host lifecycle is authoritative; embedded session dies with it. | ✓ |
| Attempt a graceful quit first, then force-close if it does not exit quickly | Prefer graceful teardown before escalating. | |
| Stay alive and try to recover into a standalone top-level window | Reparent/fallback rescue behavior. | |

**User's choice:** Exit immediately without any standalone fallback or save prompt.
**Notes:** The embedded session must never escape the host lifecycle into a rescue window.

---

## the agent's Discretion

- Exact Win32/Avalonia event hooks for activation, deactivation, and focus return.
- Exact mechanism for clearing camera/gizmo drag state when the host app deactivates.
- Exact persistence/file naming strategy for separating embedded layout state from standalone layout state.

## Deferred Ideas

- Launch-time JSONL auto-import and startup import error handling — Phase 45
- Launch-time live refresh flags and observer wiring — Phase 46
- Bundled example JSONL and richer host-side JSONL/live-refresh status messaging — Phase 47
- Rich host/global shortcuts or explicit host-side focus-return controls — out of Phase 44 scope
