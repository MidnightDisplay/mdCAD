---
gsd_state_version: 1.0
milestone: v1.8
milestone_name: Embeddable Windows JSONL Viewer
status: ready_for_next_phase
stopped_at: Completed Phase 44 execution
last_updated: "2026-05-14T18:50:45.3434084+01:00"
last_activity: 2026-05-14 -- Completed Phase 44 embedded lifecycle and layout execution
progress:
  total_phases: 5
  completed_phases: 2
  total_plans: 7
  completed_plans: 7
  percent: 100
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-14)

**Core value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.
**Current focus:** Phase 45 — Startup JSONL Auto-Import (ready to plan)
**Locked backend:** `cglm 0.9.6`
**Adoption mode:** direct cglm adoption through a thin project-owned math entrypoint

## Current Position

Phase: 44 (Embedded Resize, Focus & Viewer Layout) — COMPLETE
Plan: 4 of 4 complete
Status: Ready for Phase 45 planning after Phase 44 manual verification
Last activity: 2026-05-14 -- Completed Phase 44 embedded lifecycle and layout execution

Progress: [██████████] 100%

## Milestone Scope

- Milestone v1.8 goal: let a Windows host launch mdCAD as an embeddable child viewer that can auto-open a large flat JSONL file and optionally live-refresh it from the command line.
- Scope includes a minimal Avalonia host example, Win32 child HWND integration contract, and embedded resize/focus/input behavior.
- Roadmap v1.8 spans phases 43-47 across embedding bootstrap, embedded interaction, startup JSONL import, live refresh, and sample-host proof.
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

### Pending Todos

- Run the Phase 44 manual checklist rows for resize, focus, teardown, and embedded layout isolation.
- Plan and execute Phase 45 to add startup JSONL auto-import on top of the completed Phase 44 embedding foundation.
- Keep deferred `Clear Scene` lifecycle parity and Phase 41/42 Nyquist backfill explicit unless embedding work exposes them as blockers.

### Blockers/Concerns

- No blocking issues.
- Manual live verification is still needed for the Phase 44 checklist rows that cannot be automated in CTest.

### Quick Tasks Completed

| Date       | ID         | Task | Status | Commit |
|------------|------------|------|--------|--------|
| 2026-04-14 | 260414-mkp | Enable 4x MSAA for main viewport only (pick buffer unchanged) | done | `8048247` |
| 2026-05-05 | 260505-p42v | Create missing Phase 42 verification artifact | done | `e2ce8fe` |

## Session Continuity

Last session: 2026-05-14T18:50:45.3434084+01:00
Stopped at: Completed Phase 44 execution
Resume file: .planning/ROADMAP.md
