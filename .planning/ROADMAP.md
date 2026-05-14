# Milestone v1.8: Embeddable Windows JSONL Viewer

**Status:** ACTIVE
**Phases:** 43-47
**Total Plans:** 11

## Overview

v1.8 adds a Windows-only embedding workflow where an external host launches mdCAD as a true child window, can optionally auto-open a large flat JSONL file at startup, and can opt into existing linked refresh behavior without expanding into IPC, in-process embedding, or cross-platform hosting.

The roadmap follows the research risk order: prove strict child-window startup first, close resize/focus/input correctness second, then add startup JSONL import, then launch-time live refresh, and finish with a minimal Avalonia sample host that acts as the end-to-end proof harness.

## Phases

### Phase 43: Embed Contract & Child-Window Bootstrap

**Goal**: Developer can launch mdCAD as a strict embedded child window from a Windows host.  
**Depends on**: Phase 42  
**Plans**: 3/3 plans complete  
**Requirements**: EMBD-01, EMBD-02, EMBD-03, HOST-01  
**Status**: Complete (verified 2026-05-14)

**Success Criteria:**
1. Developer can launch mdCAD in embedded mode by passing a parent HWND from the sample Avalonia host.
2. mdCAD appears inside the host's native control region as a true child window, not as a separate standalone top-level window.
3. Missing, invalid, or incompatible embedded-mode arguments fail fast with a clear startup error and no silent fallback.

**Details:**
- Add the launch-config contract for `--embedded` and `--parent-hwnd`.
- Create the Win32 child-window path at the native window-creation seam instead of relying on late `SetParent`.
- Prove the sample host can build and launch the embedded viewer surface.
- Verification passed in `.planning/phases/43-embed-contract-child-window-bootstrap/43-VERIFICATION.md` with all 6 must-haves satisfied.

Plans:
- [x] 43-01-PLAN.md — Pin Sokol and lock the strict embed launch parser contract.
- [x] 43-02-PLAN.md — Create the true child-window bootstrap path and embedded viewer defaults.
- [x] 43-03-PLAN.md — Build the minimal Avalonia host and capture attach/failure smoke evidence.

### Phase 44: Embedded Resize, Focus & Viewer Layout

**Goal**: Users can interact with the embedded viewer correctly inside the host lifecycle.  
**Depends on**: Phase 43  
**Plans**: 8/8 plans complete  
**Requirements**: EMBD-04, INPT-01, INPT-02, INPT-03, INPT-04
**Status**: Execution complete; final human UAT rerun pending

**Success Criteria:**
1. Resizing the host control resizes the embedded mdCAD render surface without clipped, stale, or incorrect viewport behavior.
2. Clicking into the embedded viewer immediately gives mdCAD keyboard and mouse control without host interference.
3. Moving focus between host UI and mdCAD does not leave stuck capture, stuck drags, or broken input state.
4. The embedded viewer uses a viewer-first layout that fits the hosted region, and closing the host/control shuts mdCAD down cleanly.

**Details:**
- Keep the existing input/render pipeline authoritative while adding embedded focus and lifecycle glue.
- Add embedded-layout policy so the hosted viewer fills the region cleanly.
- Harden close/orphan behavior before any startup import work begins.

Plans:
- [x] 44-01-PLAN.md — Front-load Wave 0 input/layout validation seams and the Phase 44 manual checklist.
- [x] 44-02-PLAN.md — Harden native embedded focus, capture-loss, and drag-cancel behavior inside mdCAD.
- [x] 44-03-PLAN.md — Extend the Avalonia host for deterministic resize/orphan teardown proof.
- [x] 44-04-PLAN.md — Separate embedded layout persistence and seed the approved viewer-first dock recipe.
- [x] 44-05-PLAN.md — Unblock reducer-owned embedded keyboard shortcuts after first-click ownership.
- [x] 44-06-PLAN.md — Prefer graceful embedded self-exit and bind Scenario 7 to the runtime embedded ini path.
- [x] 44-07-PLAN.md — Acquire deliberate-click keyboard focus through mdCAD's native child-window activation path and add host chrome focus return.
- [x] 44-08-PLAN.md — Invalidate the placeholder on normal host close before waiting so embedded layout flushes before fallback cleanup.

### Phase 45: Startup JSONL Auto-Import

**Goal**: Developer can launch directly into a large flat JSONL view without breaking the embedded session.  
**Depends on**: Phase 44  
**Plans**: 0 plans  
**Requirements**: JSON-01, JSON-03

**Success Criteria:**
1. Launching mdCAD with an absolute JSONL path auto-imports the file at startup through the large flat dump workflow.
2. If startup JSONL import fails, the user keeps a usable embedded viewer and sees a clear error state instead of a crash or silent blank failure.

**Details:**
- Reuse the existing large flat JSONL import job rather than creating a separate embedded importer.
- Add a non-UI startup import controller so embedded launch does not depend on Scene Hierarchy UI state.

### Phase 46: Launch-Time Live Refresh

**Goal**: Developer can opt into startup-linked refresh without changing existing default refresh behavior.  
**Depends on**: Phase 45  
**Plans**: 0 plans  
**Requirements**: JSON-02, JSON-04

**Success Criteria:**
1. When launched with the explicit live-refresh flag, the embedded viewer updates from changes to the startup JSONL using the existing linked refresh behavior.
2. Without that explicit flag, startup import does not enable live refresh by default and other existing refresh workflows keep their current behavior.

**Details:**
- Keep launch-time live refresh an explicit opt-in.
- Reuse existing observer metadata and commit-on-success semantics.

### Phase 47: Sample Host Workflow Proof

**Goal**: Developer can use the sample host as the end-to-end proof harness for embedded launch workflows.  
**Depends on**: Phase 46  
**Plans**: 0 plans  
**Requirements**: HOST-02, HOST-03, HOST-04

**Success Criteria:**
1. The sample host resolves a bundled example JSONL from `resources/examples`, converts it to an absolute path, and launches mdCAD with it.
2. The sample host shows clear session status text for launch, attach, JSONL import, and live-refresh state.
3. Developer can repeatedly launch, resize, focus, and close the embedded mdCAD session from the sample host without leaving orphaned processes.

**Details:**
- Keep the Avalonia host minimal and workflow-focused.
- Use the sample as the conformance harness for lifecycle and failure-path validation, not as a productized shell.

---

## Coverage Map

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
| JSON-01 | Phase 45 | Pending |
| JSON-02 | Phase 46 | Pending |
| JSON-03 | Phase 45 | Pending |
| JSON-04 | Phase 46 | Pending |
| HOST-01 | Phase 43 | Complete |
| HOST-02 | Phase 47 | Pending |
| HOST-03 | Phase 47 | Pending |
| HOST-04 | Phase 47 | Pending |

**Coverage:**
- v1 requirements: 16 total
- Mapped to phases: 16
- Unmapped: 0

---

## Milestone Summary

**Key Decisions:**
- Build embedding as a strict Windows child-HWND launch path instead of a reparented standalone window hack.
- Keep the host/viewer boundary CLI-driven with mdCAD remaining a separate process and no new IPC surface.
- Reuse the existing large flat JSONL import and linked refresh semantics instead of redesigning refresh behavior.

**Deferred Scope:**
- In-process / DLL / SDK embedding
- Rich host-to-viewer IPC
- Cross-platform host parity
- Renderer rewrites or backend swaps
- Refresh architecture redesign

---

_For current project status, see .planning/STATE.md_
