# Architecture Patterns

**Project:** mdCAD v1.8 — Embeddable Windows JSONL Viewer  
**Domain:** Windows-hosted embeddable viewer for an existing native CAD app  
**Researched:** 2026-05-14  
**Confidence:** MEDIUM-HIGH

## Recommended Architecture

Treat v1.8 as a **launch/windowing integration layer** around the existing mdCAD runtime, not as a renderer rewrite or a host API redesign.

Reuse these existing systems as much as possible:
- app lifecycle in `src/app.c` (`init -> frame -> event -> sokol_main`)
- ECS scene/update/render flow
- existing viewport/picking/gizmo/input pipeline
- existing large flat JSONL import job
- existing linked JSONL observer/refresh system

Add only three new runtime seams:
1. Launch config parser
2. Windows embedding adapter
3. Startup flat-JSONL import controller

The highest-risk area is **Win32 child-window creation under Sokol**, not JSONL import.

---

## Existing Integration Points

| Area | Current role | Why it matters for v1.8 |
|------|--------------|-------------------------|
| `src/app.c` | Single app entry/lifecycle owner | Best place to own launch mode and startup orchestration |
| `src/ui/ui_scene_hierarchy.h` | Current flat JSONL import UI flow | Good reference for existing behavior, but wrong owner for launch-time import |
| `src/jsonl_import_job.h` | Chunked flat JSONL import job | Reuse directly for startup auto-import |
| `src/jsonl_observer_system.h` | Linked refresh/manual refresh observer logic | Reuse after launch-time import succeeds |
| `src/ui/ui_viewport.h` | Viewport sizing and input math | Resize/input behavior should stay authoritative here |
| `vendors/libsokol/sokol.c` / `sokol_app.h` | Native window creation boundary | Likely where the Windows embedding seam must live |

---

## New / Modified Components

### 1. Launch config parser
Recommended component: `src/app_launch_config.h` (or equivalent)

Suggested responsibility:
- parse `--embedded`
- parse/validate `--parent-hwnd`
- parse `--jsonl`
- parse `--jsonl-live-refresh`
- make launch mode available to `app.c` before init begins

This should be a single source of truth for startup behavior.

### 2. Windows embedding adapter
Recommended component: `src/platform/win32_embed.h/.c` (or equivalent thin Win32-only layer)

Suggested responsibility:
- validate `parent_hwnd`
- expose embedded vs standalone state
- own parent-liveness/orphan detection
- own Win32-specific sizing/focus glue

Keep Win32 child-window logic out of general app code as much as possible.

### 3. Sokol Win32 creation seam
Recommended approach: a **narrow local Sokol Win32 extension/patch** so embedded mode can create a child window from birth.

Why:
- mdCAD window creation happens before `init()`
- Sokol exposes `sapp_win32_get_hwnd()` but not a documented parent-HWND creation field
- `SetParent` is not a sufficient final architecture for correct child-window semantics

### 4. Embedded layout mode
Recommended change: add an embedded viewer layout mode that:
- skips normal dockspace/panel-heavy standalone layout
- lets the viewport fill the child client area (or nearly so)
- keeps the same viewport coordinate/input math

This is a UI policy change, not a renderer fork.

### 5. Startup flat JSONL import controller
Recommended component: `src/startup_flat_import.h/.c` or equivalent `app.c`-owned helper state

Do **not** drive startup import from `ui_scene_hierarchy_state_t`.

Instead, create a non-UI wrapper that can:
- start a flat import from an absolute path
- request live refresh opt-in when asked
- tick import progress from `frame()`
- reuse existing success/error/reset semantics

---

## Runtime vs Host Boundaries

| Concern | mdCAD runtime | Avalonia sample host |
|---------|---------------|----------------------|
| Parse embedded-mode CLI | Yes | No |
| Own scene/render/input loop | Yes | No |
| Create/attach child-window semantics | Yes | Supplies parent HWND only |
| Auto-import JSONL | Yes | Passes CLI only |
| Enable live refresh | Yes | Passes CLI only |
| Tick linked refresh | Yes | No |
| Resize child HWND | Accepts it | Yes |
| Focus child HWND on host interaction | Must handle it | Yes |
| Process launch/termination | No | Yes |
| Status UI for launch/attach | Minimal/debug only | Yes |
| Example JSONL bundling | No | Yes |

The host should own HWND/process lifecycle only. mdCAD should continue owning rendering, input, scene state, import, refresh, and shutdown behavior.

---

## Recommended Control Flow

### Standalone mode
`Process start -> sokol_main -> init -> frame -> event`

No material change.

### Embedded mode
1. Host creates a native placeholder HWND
2. Host launches `mdCAD.exe --embedded --parent-hwnd <HWND> --jsonl <abs-path> [--jsonl-live-refresh]`
3. mdCAD parses CLI into launch config
4. Sokol/Win32 layer creates mdCAD as a child HWND
5. `init()` initializes normal runtime systems
6. Embedded layout mode is selected
7. Startup import controller begins flat JSONL import if requested
8. `frame()` ticks import, observer systems, and normal viewport rendering
9. Host resizes the placeholder; mdCAD receives native resize and adapts through existing viewport/render-target paths
10. If the parent HWND disappears, mdCAD requests shutdown

---

## What Should Not Change

- ECS scene ownership
- Rendering backend selection logic
- Pick buffer / gizmo / selection architecture
- Linked JSONL observer semantics
- Existing large flat import implementation
- Manual Scene Hierarchy import workflow for standalone mode

v1.8 should add a **new launch path**, not a separate scene/import stack.

---

## Low-Risk Build Order

### Phase 1 — Launch config and child-window bootstrap
Deliver:
- CLI parser
- embedded mode flag
- parent HWND contract
- minimal child-window creation path
- simple host sample that launches and displays mdCAD

### Phase 2 — Resize, focus, and input correctness
Deliver:
- host-driven resize
- focus acquisition
- keyboard/mouse sanity
- parent-close/orphan shutdown behavior
- embedded viewer layout mode

### Phase 3 — Startup flat JSONL auto-import
Deliver:
- non-UI startup import controller
- absolute-path startup import
- launch-time success/error path

### Phase 4 — Live refresh opt-in
Deliver:
- `--jsonl-live-refresh`
- observer contract wiring
- embedded-mode refresh proof

### Phase 5 — Sample host polish and docs
Deliver:
- bundled example JSONL
- status messaging
- build/run instructions
- cleanup behavior
- integration validation notes

---

## Anti-Patterns

- Putting embedded startup import logic inside `ui_scene_hierarchy`
- Treating embedding as "the host controls mdCAD"
- Relying on late `SetParent` as the final design
- Mixing the Avalonia sample into the core native build/test flow

---

## Sources

- `src/app.c`
- `src/ui/ui_scene_hierarchy.h`
- `src/jsonl_import_job.h`
- `src/jsonl_observer_system.h`
- `src/ui/ui_viewport.h`
- `src/CMakeLists.txt`
- `docs/VULKAN_WINDOWS.md`
- Sokol `sokol_app.h`: https://github.com/floooh/sokol
- Microsoft `SetParent`: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setparent
- Microsoft child-window docs: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features
- Avalonia `NativeControlHost`: https://api-docs.avaloniaui.net/docs/T_Avalonia_Controls_NativeControlHost
