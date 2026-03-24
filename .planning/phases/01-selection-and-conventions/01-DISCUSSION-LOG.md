# Phase 1: Selection and Conventions - Discussion Log

**Gathered:** 2026-03-24
**Status:** Complete

## Discussion Summary

- Locked the backend to `cglm` `0.9.6`
- Rejected carrying an active fallback candidate through planning
- Chose direct `cglm` adoption over a long-lived compatibility facade
- Chose performance-first alignment policy with global enforcement
- Chose a new project-wide convention of column-major, right-handed, clip depth `0..1`
- Chose header-only / inline vendoring with minimal CMake churn

## Backend commitment

### Q1
**Question:** How hard should Phase 1 lock the library choice?

**Options presented:**
- `Lock cglm now`
- `Lock cglm, but require one fallback checkpoint`
- `Keep the choice open until after vendoring`

**Selected:** `Lock cglm now`

### Q2
**Question:** What should count as a valid blocker strong enough to reopen the choice?

**Options presented:**
- `Build or license blocker only`
- `Build or convention blocker`
- `Any meaningful perf or ergonomics concern`

**Selected:** `Build or license blocker only`

### Q3
**Question:** How tightly should Phase 1 pin the adopted `cglm` version?

**Options presented:**
- `Pin exact version 0.9.6`
- `Pin the 0.9.x line`
- `Track a specific upstream commit`

**Selected:** `Pin exact version 0.9.6`

### Q4
**Question:** How should the context treat alternative libraries once `cglm` is locked?

**Options presented:**
- `No active fallback`
- `Keep one backup candidate noted`
- `Document alternatives only as background`

**Selected:** `No active fallback`

## mdCAD-facing API shape

### Q1
**Question:** At the compatibility boundary, what should migrated mdCAD code see?

**Options presented:**
- `Keep project-owned vec3_t / mat4_t / ray_t names and struct-style access`
- `Keep project-owned types, but rename them early to a new mdCAD math namespace`
- `Allow raw cglm types in migrated code`

**Selected:** `Allow raw cglm types in migrated code`

### Q2
**Question:** If raw `cglm` types are allowed, what remains project-owned?

**Options presented:**
- `Only convention/config headers stay local`
- `Project-owned wrappers for risky operations only`
- `No real wrapper layer`

**Selected:** `No real wrapper layer`

### Q3
**Question:** How should the `no real wrapper layer` choice be interpreted against the current roadmap?

**Options presented:**
- `Revise the roadmap direction`
- `Keep only a thin project-owned boundary`
- `Keep the existing roadmap instead`

**Selected:** `Revise the roadmap direction`

### Q4
**Question:** Which `cglm` API family should migrated mdCAD code use as the primary style?

**Options presented:**
- `Struct API`
- `Array/inline API`
- `Mixed by subsystem`

**Selected:** `Mixed by subsystem`

## Alignment and SIMD posture

### Q1
**Question:** What should Phase 1 optimize for when `cglm` alignment requirements collide with existing mdCAD layouts?

**Options presented:**
- `Safety first`
- `Performance first`
- `Split by subsystem`

**Selected:** `Performance first`

### Q2
**Question:** How far can that go in practice?

**Options presented:**
- `Refactor data layouts if needed`
- `Refactor only runtime-hot structs`
- `Avoid layout refactors; use copies/adapters instead`

**Selected:** `Refactor data layouts if needed`

### Q3
**Question:** Once that path is taken, how explicit should the alignment policy be?

**Options presented:**
- `One global policy with documented rules and assertions`
- `Per-subsystem policy`
- `Implicit cglm defaults`

**Selected:** `One global policy with documented rules and assertions`

### Q4
**Question:** If a supported native platform cannot meet that policy cleanly, what is the fallback?

**Options presented:**
- `Block the migration slice until it is solved`
- `Allow temporary per-platform fallback`
- `Automatically drop to safer unaligned mode`

**Selected:** `Block the migration slice until it is solved`

## Convention lock

### Q1
**Question:** Should Phase 1 preserve mdCAD's current math conventions exactly, or use the migration to change them?

**Options presented:**
- `Preserve current conventions exactly`
- `Preserve behavior, but allow internal representation changes`
- `Use the migration to standardize on cglm defaults even if behavior changes`

**Selected:** `Use the migration to standardize on cglm defaults even if behavior changes`

### Q2
**Question:** What should the new global convention baseline be?

**Options presented:**
- `OpenGL-style global convention`
- `Modern GPU-style global convention`
- `Backend-native per platform`

**Selected:** `Modern GPU-style global convention`

### Q3
**Question:** How hard should that convention be enforced across mdCAD?

**Options presented:**
- `One project-wide convention, no per-backend semantic forks`
- `One convention for native, looser rules for web/mobile later`
- `Per-backend semantic freedom`

**Selected:** `One project-wide convention, no per-backend semantic forks`

### Q4
**Question:** When the new convention changes visible behavior during migration, what should win?

**Options presented:**
- `New convention wins immediately`
- `Visible behavior should still be normalized during rollout`
- `Case-by-case by subsystem`

**Selected:** `Visible behavior should still be normalized during rollout`

## Vendor integration mode

### Q1
**Question:** How should mdCAD bring `cglm` into the build for the first real adoption pass?

**Options presented:**
- `Header-only / inline first`
- `Linked call API from the start`
- `Allow both modes from Phase 1`

**Selected:** `Header-only / inline first`

### Q2
**Question:** How should that show up in the build system?

**Options presented:**
- `Minimal include-path vendoring`
- `Dedicated CMake interface target`
- `Configurable integration switch from day one`

**Selected:** `Minimal include-path vendoring`

### Q3
**Question:** Once Phase 1 starts that way, when should linked `cglm` be reconsidered?

**Options presented:**
- `Only after measurable compile-size or performance pressure appears`
- `During the same milestone if implementation feels messy`
- `Keep linked mode on the table immediately`

**Selected:** `Only after measurable compile-size or performance pressure appears`

## Notes

- The user intentionally overrode the current wrapper-first assumption in `.planning/REQUIREMENTS.md` and `.planning/ROADMAP.md`.
- The user also accepted an internal convention change away from the exact current `src/math3d.h` semantics, but still expects user-visible runtime behavior to stay stable during staged rollout.

---
*Phase: 01-selection-and-conventions*
*Discussion logged: 2026-03-24*
