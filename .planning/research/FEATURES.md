# Feature Landscape

**Domain:** CAD-style large geometry log workflows (v1.6 Observable Flat JSONL Import)  
**Researched:** 2026-04-27  
**Confidence:** HIGH (repo context + shipped v1.5 observer/import behavior)

## Table Stakes

Features users expect for this milestone. Missing any of these makes the feature feel incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Separate menu path: `File -> Import JSONL (Flat / Large)` | Users need explicit intent: "fast large dump import," not sketch workflow | Low | Must not replace existing JSONL Geometry Log or JSONL as Sketch entries |
| Dedicated flat import dialog | Users expect to confirm scale/rotation/center/color behavior before heavy import | Medium | Reuse proven JSONL options shape; keep terminology clear that output is non-sketch entities |
| Import into one anchor entity | Large dump workflows expect one controllable root for selection, hide/show, delete, and refresh | Medium | No per-entry sketch conversion; plain scene entities only |
| Optional observer link at import time | Users need live-update option without forcing it for huge files | Medium | Opt-in toggle in dialog; link metadata attaches to anchor |
| Manual refresh (`Re-import now`) | CAD operators expect deterministic "pull latest file now" control | Medium | Must work even when auto-observe is off/disabled |
| Transactional refresh semantics | Users expect "all-or-nothing" replacement, never partial corruption on parse/read failure | High | Keep last good anchor content on failure |
| Stable behavior on huge files (hundreds of thousands of geometries) | Core reason for milestone | High | Prioritize responsiveness and no UI lock/crash during import/refresh |

## Differentiators

Features that make this especially strong for large-geometry log workflows.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Flat refresh replaces only anchor subtree (not sketch/script pipeline) | Much faster and lower overhead for huge logs | High | Primary performance differentiator vs sketch-observer flow |
| Observer defaults tuned for safety on huge logs | Avoid runaway retries and UI spam during file-lock churn | Medium | Reuse retry/debounce/auto-disable pattern already proven in observer flow |
| Import mode explicitly branded for "Large Geometry Dumps" | Reduces wrong-path usage and support confusion | Low | UX clarity is a practical differentiator |
| Clear status + recent observer messages on anchor | Operators can diagnose refresh failures quickly | Medium | Reuse "latest-two messages" style behavior |

## Anti-Features

Explicitly avoid these in v1.6.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|--------------------|
| Auto-convert flat import into sketch entities | Reintroduces heavy sketch/script overhead; defeats milestone purpose | Keep flat mode strictly plain scene entities |
| Silent partial refresh on parse errors | Causes mixed old/new geometry and trust loss | Keep transactional replace-or-rollback |
| Forced observer ON by default for flat mode | Dangerous on giant files; can surprise users with heavy background reparses | Make observer opt-in in flat import dialog |
| Per-entry anchor hierarchy in flat mode | Adds hierarchy bloat and slows large-scene operations | Single anchor with flat children only |
| Adding constraint inference/authoring in this milestone | Scope creep; unrelated to large dump refresh performance | Keep import unconstrained and non-sketch |

## Feature Dependencies

```text
Separate menu entry
  -> Dedicated flat import dialog
    -> Import options capture (units/transform/color + observe opt-in)
      -> Flat import job (plain entities under one anchor)
        -> Anchor metadata/link persistence
          -> Manual refresh
            -> Auto-observe refresh loop
              -> Transactional replacement + retry/debounce/auto-disable messaging
```

## MVP Recommendation (v1.6)

Prioritize:
1. Separate flat import menu + dialog (explicit mode separation)
2. One-anchor flat entity import with large-file-safe execution
3. Optional observer link with manual refresh and transactional re-import
4. Reuse observer retry/debounce/auto-disable + recent-message UX on anchor

Defer:
- Any sketch/script integration for flat mode
- Smart diff/patch refresh (full anchor replace is acceptable for v1.6)
- New geometry semantics beyond existing JSONL type handling

## Scoping Notes for /gsd-new-milestone

Use requirement slices like:
- **FIMP-01**: User can launch flat JSONL import from dedicated File menu action.
- **FIMP-02**: Import creates one anchor with plain scene-entity geometry (non-sketch).
- **FIMP-03**: Import dialog supports transform/color options and observer opt-in.
- **FIMP-04**: Manual refresh re-imports source into same anchor transactionally.
- **FIMP-05**: Auto-observe refresh uses retry/debounce/auto-disable safety semantics.
- **FIMP-06**: Huge-file workflow remains responsive and stable under repeated refresh.

## Sources

- `.planning/PROJECT.md` (v1.6 scope and goals)
- `.planning/STATE.md` (milestone focus and constraints)
- `.planning/phases/35-.../35-CONTEXT.md` (observer semantics baseline)
- `.planning/phases/35-.../35-UAT.md` (observer UX/behavior validation baseline)
- `src/ui/ui_scene_hierarchy.h` (existing import menu/dialog patterns)
- `src/jsonl_import_job.h` (current JSONL geometry-log import behavior)
- `src/components/jsonl_observer_comp.h` (observer state model)
- `src/jsonl_observer_system.h` (manual/auto reparse, retry/debounce behavior)
