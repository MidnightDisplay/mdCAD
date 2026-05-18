---
gsd_state_version: 1.0
milestone: v1.9
milestone_name: milestone
status: executing
stopped_at: Phase 54 plan 01 complete
last_updated: "2026-05-18T18:25:25.2437251+01:00"
last_activity: 2026-05-18 -- Completed Phase 54 plan 01 README contract rewrite
progress:
  total_phases: 6
  completed_phases: 5
  total_plans: 18
  completed_plans: 16
  percent: 89
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-15)

**Core value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.
**Current focus:** Phase 54 — docs-and-onboarding-truthfulness
**Locked backend:** `cglm 0.9.6`
**Adoption mode:** direct cglm adoption through a thin project-owned math entrypoint

## Current Position

Phase: 54 (docs-and-onboarding-truthfulness) — EXECUTING
Plan: 2 of 3
Status: Phase 54 plan 01 complete; plan 02 next
Last activity: 2026-05-18 -- Completed Phase 54 plan 01 README contract rewrite

Progress: [█████████░] 89%

## Milestone Scope

- Milestone v1.9 goal: let a plain `net10.0` Avalonia host reference the reusable control directly while keeping the embedded mdCAD viewer itself Windows-only.
- Scope includes host-facing TFM compatibility widening, a safe non-Windows placeholder contract, and preservation of the existing Windows child-HWND embedding/runtime packaging path.
- Roadmap v1.9 spans phases 50-54 plus inserted Phase 52.1 across backend extraction, unsupported-platform contract, plain-net10 compatibility, Windows runtime refresh automation, consumer proof, and onboarding truthfulness.
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
- [Phase 48]: Phase 48 starts from a Windows-only Avalonia UserControl with StyledProperty launch configuration and an internal NativeControlHost placeholder seam — This preserves the XAML-drop consumer contract while keeping the proven Win32 child-HWND wiring library-owned for later runtime packaging and relaunch work.
- [Phase 48]: Runtime packaging now resolves mdCAD only from AppContext.BaseDirectory\\mdcad-runtime and ships a committed curated win-x64 bundle — This removes repo-root discovery, keeps the control self-contained for ProjectReference consumers, and preserves imgui.embedded.ini persistence through a runtime-root working directory in later lifecycle work.
- [Phase 48]: The reusable control keeps a bare sealed surface by default and exposes the Phase 47 launch/status chrome only through explicit diagnostic mode while the sample host stays the scenario harness — This preserves the minimal embeddable API while still giving the in-repo consumer a truthful proof surface for start, stop, relaunch, and warning behavior.
- [Phase 49]: The repo now ships both a diagnostic harness and a separate minimal sealed consumer sample, with the control quickstart living beside the reusable control project — This keeps external onboarding lightweight while preserving the richer sample host as the proof and debugging surface.
- [Phase 50]: Windows lifecycle ownership now lives behind `IMdCadEmbedBackend`, while `MdCadEmbeddedControl` keeps the public properties, warning/status UI, and presentation-mode surface. — This isolates Win32/process state without changing the consumer-facing control contract.
- [Phase 50]: `MdCadSessionCoordinator` now starts, stops, and recreates sessions through backend delegates instead of shell-owned Win32 state. — Keeping the same generation gate and relaunch ordering preserves the existing stop/recreate/restart behavior while decoupling the coordinator from the shell implementation.
- [Phase 50]: Relaunch-safe proof now explicitly pins the current placeholder HWND in repeated backend start-info generation, and the Windows diagnostic host checklist is approved as PASS. — This closes the backend extraction loop without introducing a new proof surface or widening platform scope.
- [Phase 51]: Unsupported hosts should show one consistent Windows-only runtime truth in both presentation modes, with `sealed` minimal and `diagnostic` more explicit. — This keeps the default surface lightweight while making unsupported runtime behavior visible immediately.
- [Phase 51]: Unsupported `StartAsync()` must fail immediately with the same canonical message the placeholder shows, while `StopAsync()` remains a safe no-op. — Programmatic hosts get deterministic behavior without implying cross-platform runtime support.
- [Phase 51]: The public control shell now selects backends through `MdCadEmbedBackendFactory` and treats `StartBlockedReason` as the primary warning truth, while diagnostic JSONL/live-refresh lines remain informational-only. — This keeps unsupported runtime behavior explicit without changing the public API or the locked Windows launch path.
- [Phase 52]: `samples/avalonia-host-minimal` is part of Phase 52 itself as the smallest plain-`net10.0` proof host, while the Windows diagnostic harness remains a separate regression consumer. — This gives the compatibility widening a real plain-host proof surface without pulling broader Phase 53 proof scope into this phase.
- [Phase 52]: The reusable control and minimal proof host now both target plain `net10.0`, while the Windows diagnostic harness remains Windows-targeted and continues to validate the runtime-specific path separately. — This keeps compile-time compatibility widening distinct from Windows runtime proof.
- [Phase 53]: Consumer proof is closed only when the plain `net10.0` build proof, automated Windows preflight, and approved manual host lifecycle proof all remain separate and green. — This preserves the compile-vs-runtime support boundary for the docs and onboarding phase.

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
- Phase 49 added: Add minimal sealed Avalonia host sample and QUICKSTART for reusable control.
- Phase 50 planned: 3 verified execution plans plus research and validation artifacts are ready for execution.
- Phase 52.1 inserted after Phase 52: Automate Windows runtime refresh from build-vulkan with a dotnet-managed post-build helper (URGENT)
- Phase 50 completed: backend extraction, automated relaunch lock, and approved Windows diagnostic-host proof are recorded.
- Phase 51 planned: 3 verified execution plans plus context, research, and validation artifacts are ready for execution.
- Phase 51 plan 01 completed: Wave 0 unsupported backend/coordinator contract tests are committed and red for the intended missing seams.
- Phase 51 plan 02 completed: Internal unsupported backend selection, canonical message ownership, and blocked coordinator behavior are implemented and green.
- Phase 51 completed: unsupported backend selection, blocked coordinator behavior, and truthful shell messaging are recorded.
- Phase 52 planned: 3 verified execution plans plus research and validation artifacts are ready for execution.
- Phase 52 plan 01 completed: the reusable control now targets plain `net10.0` and the immediate regression bundle remains green.
- Phase 52 plan 02 completed: the minimal proof host now targets plain `net10.0` and its startup smoke is approved.
- Phase 52 completed: the control and minimal proof host now target plain `net10.0`, and the Windows regression lane remains green.
- Phase 52.1 planned: 3 verified execution plans plus research and validation artifacts are ready for execution.
- Phase 52.1 plan 01 completed: the runtime refresh helper foundation, targeted tests, and green validation row 52.1-01-01 are recorded.
- Phase 52.1 plan 02 completed: the control project now exposes an opt-in post-build refresh target and keeps output ini state preserved.
- Phase 52.1 completed: the repo now has approved host-level proof for the automated Windows runtime refresh chain.
- Phase 53 planned: 3 verified execution plans plus research and validation artifacts are ready for execution.
- Phase 53 plan 01 completed: the minimal plain-net10 proof host still builds cleanly and validation row 53-01-01 is green.
- Phase 53 plan 02 completed: the automated Windows preflight stayed green and validation row 53-02-01 is recorded.
- Phase 53 completed: plain net10 proof, automated Windows preflight, and approved manual lifecycle proof are all recorded.
- Phase 54 planned: 3 verified execution plans plus research and validation artifacts are ready for execution.
- Phase 54 plan 01 completed: README now documents the plain-net10 host contract, the Windows runtime proof surface, and unsupported-platform truth.

### Pending Todos

- Execute Phase 54 plan 02 next so QUICKSTART and the minimal sample align with the updated support boundary.
- Decide whether the accepted v1.8 audit gaps should become follow-up validation/cleanup work in this milestone or remain deferred tech debt.
- Keep deferred `Clear Scene` lifecycle parity and Phase 41/42 Nyquist backfill explicit unless the new milestone exposes them as blockers.

### Blockers/Concerns

- The control owns real Win32 seams, so widening compile-time compatibility must not accidentally imply cross-platform runtime embedding support.
- Phase 53 must prove the refreshed runtime through real consumer flows without reopening compile-time compatibility or broadening prematurely into docs/onboarding scope.

### Quick Tasks Completed

| Date       | ID         | Task | Status | Commit |
|------------|------------|------|--------|--------|
| 2026-04-14 | 260414-mkp | Enable 4x MSAA for main viewport only (pick buffer unchanged) | done | `8048247` |
| 2026-05-05 | 260505-p42v | Create missing Phase 42 verification artifact | done | `e2ce8fe` |
| 2026-05-15 | 260515-nvk | Retarget Avalonia control and sample hosts to net10 for net10 host compatibility | done | `d847033` |

## Performance Metrics

| Plan | Duration | Tasks | Files |
|------|----------|-------|-------|
| Phase 48 P01 | 8 min | 2 tasks | 7 files |
| Phase 48 P02 | 8 min | 2 tasks | 7 files |
| Phase 48 P03 | 12 min | 2 tasks | 9 files |
| Phase 48 P04 | 7 min | 3 tasks | 5 files |
| Phase 49 P01 | 8 min | 1 tasks | 11 files |

## Session Continuity

Last session: 2026-05-18T18:25:25.2437251+01:00
Stopped at: Phase 54 plan 01 complete
Resume file: .planning/phases/54-docs-and-onboarding-truthfulness/54-02-PLAN.md
