# Phase 3 Plan: macOS Core Transform Migration

**Created:** 2026-03-24
**Status:** Ready for execution planning review

## Scope

Migrate the orbit camera, app-side viewport matrices, and ECS transform/world-point hot paths to `cglm` on the primary macOS workflow while preserving visible parity.

## Wave Summary

1. **Wave 1 / 03-01:** Move orbit camera and app-side view/projection/MVP construction to `cglm` and freeze legacy parity baselines in the harness.
2. **Wave 2 / 03-02:** Migrate `TransformComp` composition and ECS world-point application to `cglm` while keeping cached storage stable.
3. **Wave 3 / 03-03:** Lock a repeatable macOS parity workflow around the migrated production path and document the smoke checklist.

## Source of Truth

- `.planning/phases/03-macos-core-transform-migration/03-DISCUSSION-LOG.md`
- `.planning/phases/03-macos-core-transform-migration/03-CONTEXT.md`
- `.planning/phases/03-macos-core-transform-migration/03-RESEARCH.md`
- `.planning/phases/03-macos-core-transform-migration/03-VALIDATION.md`
- `.planning/phases/03-macos-core-transform-migration/03-01-PLAN.md`
- `.planning/phases/03-macos-core-transform-migration/03-02-PLAN.md`
- `.planning/phases/03-macos-core-transform-migration/03-03-PLAN.md`

## Review Focus

- Camera and app-side matrix construction must genuinely originate from `cglm`, not from hidden fresh `math3d.h` recomputation.
- `TransformComp` and ECS world-point migration should change compute paths without forcing a broad storage or render-struct rewrite in the same phase.
- Harness comparisons must stay production-vs-legacy, and the macOS smoke workflow must be concrete enough for another agent to run directly.
