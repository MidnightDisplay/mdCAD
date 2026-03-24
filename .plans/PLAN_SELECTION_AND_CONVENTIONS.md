# Plan: Phase 1 Selection and Conventions

This repo keeps user-reviewable plan files in `.plans/`, while GSD execution consumes phase plans from `.planning/phases/`.

Phase 1 planning artifacts:

- `.planning/phases/01-selection-and-conventions/01-RESEARCH.md` — phase-specific research and validation architecture
- `.planning/phases/01-selection-and-conventions/01-VALIDATION.md` — validation contract for the phase
- `.planning/phases/01-selection-and-conventions/01-01-PLAN.md` — reconcile roadmap/requirements/docs with the locked `cglm` direct-adoption decision
- `.planning/phases/01-selection-and-conventions/01-02-PLAN.md` — vendor `cglm` `0.9.6`, wire minimal build integration, and prove compilation
- `.planning/phases/01-selection-and-conventions/01-03-PLAN.md` — define the convention/alignment contract and wire it into the `cglm` entrypoint

Execution order:

1. `01-01-PLAN.md`
2. `01-02-PLAN.md`
3. `01-03-PLAN.md`

Key locked outcomes:

- `cglm` `0.9.6` is the chosen backend
- Direct `cglm` adoption replaces the previously planned compatibility facade
- The migration standardizes on column-major, right-handed, clip depth `0..1`
- Alignment policy is performance-first and globally enforced
- Initial integration remains header-only with minimal CMake churn

Use the phase plan files above as the executable source of truth for implementation.
