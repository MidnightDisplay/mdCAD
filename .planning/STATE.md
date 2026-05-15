---
gsd_state_version: 1.0
milestone: v1.8
milestone_name: milestone
status: planning
stopped_at: Phase 48 added; ready for planning
last_updated: "2026-05-15T12:11:18.6856201+01:00"
last_activity: 2026-05-15 -- Phase 48 added to the roadmap
progress:
  total_phases: 6
  completed_phases: 5
  total_plans: 20
  completed_plans: 20
  percent: 83
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-14)

**Core value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.
**Current focus:** Phase 48 planning
**Locked backend:** `cglm 0.9.6`
**Adoption mode:** direct cglm adoption through a thin project-owned math entrypoint

## Current Position

Phase: 48 (reusable-avalonia-mdcad-user-control) — NOT PLANNED
Plan: 0 of 0
Status: Phase 48 added; ready for planning
Last activity: 2026-05-15 -- Phase 48 added to the roadmap

Progress: [████████░░] 83%

## Milestone Scope

- Milestone v1.8 goal: let a Windows host launch mdCAD as an embeddable child viewer that can auto-open a large flat JSONL file, optionally live-refresh it from the command line, and now evolve that workflow toward a reusable Avalonia control for external Windows apps.
- Scope includes the minimal Avalonia host example, Win32 child HWND integration contract, embedded resize/focus/input behavior, and the new reusable-control packaging phase.
- Roadmap v1.8 spans phases 43-48 across embedding bootstrap, embedded interaction, startup JSONL import, live refresh, sample-host proof, and reusable Avalonia control planning.
- Existing `Clear Scene` lifecycle parity and Nyquist validation backfill debt remain explicitly deferred unless this milestone exposes them as blockers.

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Milestone v1.4]: Continue phase numbering from 25; active roadmap starts at Phase 26.
- [Phase 35]: Linked JSONL observer metadata persists on imported content and refresh uses commit-on-success semantics.
- [Phase 35]: Linked JSONL observer default remains OFF unless the user explicitly enables it.
- [Phases 38-39]: Flat import refresh uses same-anchor transactional replacement plus debounce/retry/auto-disable safety behavior.
- [Phase 40]: Large repeated flat refreshes must preserve hierarchy, selection, and interaction coherence.
- [Milestone v1.7]: Scope is limited to observer-enabled large flat JSONL regression closure across import, refresh, and delete lifecycle behavior.
- [Milestone v1.8]: Scope is limited to Windows child-HWND embedding, CLI launch-time JSONL viewing, and a minimal Avalonia host with no new IPC layer.
- [Phase 44]: Front-load pure embedded input/layout reducers and a manual checklist before the runtime focus/layout changes land.
- [Phase 44]: Embedded mode now persists a dedicated `imgui.embedded.ini` store and seeds a one-time DockBuilder viewer layout on first run.
- [Phase 44]: The Avalonia host now distinguishes the mdCAD-side destroyed-parent quit path from the host-side `destroy-after-attach` fallback cleanup.
- [Phase 44]: Embedded keyboard ownership is now claimed from mdCAD's own Win32 child-window message path instead of from the Avalonia host. — Host-side SetFocus into the external child HWND is unreliable; the child now claims focus from its own first-click native path.
- [Phase 44]: Host chrome regains focus through Avalonia focus APIs only; the host never calls SetFocus or SetActiveWindow on the external mdCAD child HWND. — Separating host focus return from child focus acquisition preserves the CLI host boundary and avoids cross-process focus forcing.
- [Phase 44]: Normal host close now reuses the same placeholder invalidation seam that destroyed-parent mode already proved. — The missing normal-close flush behavior came from close ordering in the host, so the fix reuses the proven parent-invalid signal before waiting for process exit.
- [Phase 44]: The host-close path no longer performs an extra unconditional kill after the graceful wait path runs. — Keeping fallback cleanup inside the wait helper preserves a real chance for mdCAD to self-exit and flush embedded layout state before host cleanup intervenes.
- [Phase 44]: Embedded mode now routes mdCAD-global shortcuts through ImGui's Shortcut API because the shortcut block runs outside a specific window and raw key polling missed routed non-text shortcuts. — The final human UAT showed Win32 focus was fixed and only app-level global shortcuts remained broken, so the fix moved into ImGui's routing layer.
- [Phase 44]: Standalone mdCAD keeps the previous raw key polling path so the embedded shortcut fix does not change non-embedded behavior. — The remaining gap was embedded-only, so keeping the standalone path unchanged minimized regression risk.
- [Phase 44]: Normal WM_CAPTURECHANGED and WM_CANCELMODE no longer count as embedded host deactivation. — A normal mouse release also changes capture, so treating capture-change as runtime cancel cleared keyboard ownership, reverted gizmo drops, and stopped camera inertia even though focus never left mdCAD.
- [Phase 44]: The embedded child HWND now advertises WM_GETDLGCODE ownership for keyboard/dialog keys. — Claiming dialog keys from the native child-window seam keeps Tab-class input with mdCAD instead of leaving it available to the host message pump.
- [Phase 45]: `--jsonl` is a shape-validated launch flag only; bad startup paths remain runtime import failures instead of parse-time launch failures. — Preserving a usable viewer session on bad files requires the parser to validate syntax and absoluteness without probing the filesystem.
- [Phase 45]: Startup JSONL import must flow through a reusable non-UI controller that wraps `jsonl_import_job_t` with flat-import defaults and linked refresh disabled. — The existing importer already owns the large-flat ingest behavior, so Phase 45 should bridge launch config into that job instead of duplicating it behind Scene Hierarchy UI state.
- [Phase 45]: Startup JSONL import now ticks from `app.c` before the Scene Hierarchy visibility gate and marks hierarchy cache dirty on completion. — Embedded viewer-first launches can hide panels entirely, so startup import ownership must live in the app frame loop instead of in panel draw code.
- [Phase 45]: Launch-time JSONL failure now surfaces through a dismissible app-level overlay and never exits the viewer. — A missing or unreadable startup file must leave embedded and standalone sessions usable for inspection and retry.
- [Phase 46]: Startup live refresh is enabled only by the explicit `--jsonl-live-refresh` companion flag; plain startup imports stay passive because their observer metadata remains unlinked and unbaselined. — This preserves Phase 45 default-off behavior while reusing the existing linked observer runtime unchanged.
- [Phase 46]: Startup refresh status is surfaced from the imported root's observer component in `app.c` and never advances a second refresh loop. — The startup overlay may inspect runtime observer state, but `jsonl_observer_system_tick(...)` and `jsonl_observer_tick_flat_refreshes(...)` remain the only refresh drivers.
- [Phase 47]: The sample host now reports only host-owned session/request state and switches JSONL/live-refresh status lines to `viewer-managed` wording after attach. — This preserves the CLI/process boundary and avoids inventing import-confirmation IPC.
- [Phase 47]: Repeated relaunch recreates the placeholder/native host surface after teardown before starting the next embedded session. — The close/relaunch path destroys the old attach seam, so the placeholder must be renewed for the next child HWND.
- [Phase 47]: Restored MSVC full-suite validation reuses `win32_embed_test_stub.h` in standalone Windows tests that link `libsokol` outside `app.c`. — The shared embed-state symbol must still exist during full-suite verification even when the application entry point is absent.

### Roadmap Evolution

- Phase 35 added: Observable JSONL as sketch import with optional live file observer.
- Milestone v1.6 initialized: Observable Flat JSONL Import for Large Geometry Dumps.
- Roadmap v1.6 created: phases 36-40 map 11/11 requirements.
- Milestone v1.6 archived to `.planning/milestones/v1.6-{ROADMAP,REQUIREMENTS}.md`.
- Milestone v1.7 initialized: Linked Flat JSONL Large-File Refresh Stability.
- Roadmap v1.7 created: phases 41-42 map 6/6 requirements.
- Milestone v1.7 archived to `.planning/milestones/v1.7-{ROADMAP,REQUIREMENTS,MILESTONE-AUDIT}.md`.
- Milestone v1.8 initialized: Embeddable Windows JSONL Viewer.
- Roadmap v1.8 created: phases 43-47 map 16/16 requirements.
- Phase 48 added: Reusable Avalonia mdCAD user control.

### Pending Todos

- Plan Phase 48 to define the reusable Avalonia mdCAD control, external-app output packaging, and compile-time startup options for Windows host apps outside this repo.
- Keep deferred `Clear Scene` lifecycle parity and Phase 41/42 Nyquist backfill explicit unless embedding work exposes them as blockers.

### Blockers/Concerns

- No blocking issues.
- Phase 47 remains complete; Phase 48 is newly added and not planned yet.

### Quick Tasks Completed

| Date       | ID         | Task | Status | Commit |
|------------|------------|------|--------|--------|
| 2026-04-14 | 260414-mkp | Enable 4x MSAA for main viewport only (pick buffer unchanged) | done | `8048247` |
| 2026-05-05 | 260505-p42v | Create missing Phase 42 verification artifact | done | `e2ce8fe` |

## Session Continuity

Last session: 2026-05-15T12:11:18.6856201+01:00
Stopped at: Phase 48 added; ready for planning
Resume file: None
