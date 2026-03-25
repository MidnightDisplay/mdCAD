---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: Ready to execute
stopped_at: Completed 05-02-PLAN.md
last_updated: "2026-03-25T17:41:56Z"
progress:
  total_phases: 5
  completed_phases: 4
  total_plans: 15
  completed_plans: 14
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-25)

**Core value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.
**Current focus:** Phase 05 — windows-vulkan-hardening-and-performance-gates
**Locked backend:** `cglm 0.9.6`
**Adoption mode:** direct cglm adoption through a thin project-owned math entrypoint
**Fallback posture:** no active fallback candidate

## Current Position

Phase: 05 (windows-vulkan-hardening-and-performance-gates) — EXECUTING
Plan: 3 of 3

## Performance Metrics

**Velocity:**

- Total plans completed: 14
- Average duration: 1.8 min
- Total execution time: 0.2 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| Phase 01 | 3 | 5 min | 1.7 min |
| Phase 02 | 3 | 6 min | 2.0 min |

**Recent Trend:**

- Last 5 plans: 1 min, 1 min, 4 min, 1 min, 4 min
- Trend: Stable

| Phase 01 P01 | 2 min | 2 tasks | 5 files |
| Phase 01 P02 | 2 min | 2 tasks | 192 files |
| Phase 01 P03 | 1 min | 2 tasks | 3 files |
| Phase 02 P01 | 1 min | 2 tasks | 4 files |
| Phase 02 P02 | 4 min | 2 tasks | 3 files |
| Phase 02 P03 | 1 min | 2 tasks | 2 files |
| Phase 03 P01 | 1 min | 2 tasks | 3 files |
| Phase 03 P02 | 35 min | 2 tasks | 4 files |
| Phase 03 P03 | 1 min | 2 tasks | 2 files |
| Phase 04 P01 | 5min | 2 tasks | 4 files |
| Phase 04 P02 | 3 min | 2 tasks | 4 files |
| Phase 04 P03 | 3 min | 2 tasks | 4 files |
| Phase 05 P01 | 4 min | 2 tasks | 8 files |
| Phase 05 P02 | 4 min | 2 tasks | 12 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- Phase 0: Use a staged migration rather than a repo-wide swap
- Phase 0: Prioritize macOS Metal and Windows Vulkan as the native regression gates
- Phase 0: Expand math capability during migration only after parity and performance are protected
- [Phase 01]: Planning docs now pin cglm 0.9.6 with no active fallback candidate — Future waves should execute against the locked backend decision instead of reopening library selection
- [Phase 01]: Phase 2 is renamed around a thin project-owned math entrypoint instead of a compatibility facade — This keeps roadmap and requirement language aligned with direct cglm adoption
- [Phase 01]: cglm 0.9.6 is vendored locally as the first integration slice — Phase 1 keeps the dependency pinned, local, and header-only without introducing build-time fetches
- [Phase 01]: The native mdCAD target compiles vendored cglm through src/math/cglm_entry.h — A compile anchor in src/app.c proves the integration is real before convention work starts
- [Phase 01]: Migrated math semantics are now column-major, right-handed, with 0..1 clip depth — Phase 1 now has one compiled contract that future migration slices must honor
- [Phase 01]: The cglm entrypoint enforces layout compatibility with compile-time assertions — mat4 and mat4s assumptions now fail loudly if the vendor layout changes
- [Phase 02]: Comparison, validation, and benchmark helpers live in adjacent headers and accept raw float buffers so struct and array cglm call sites can share one migration surface. — This supports mixed API-family rollout without rebuilding a compatibility facade around vendor math types.
- [Phase 02]: Projection-sensitive comparison cases validate behavior-level NDC X/Y or ray outputs instead of demanding raw projection-matrix equality across clip-depth conventions. — The migration intentionally changes clip-depth semantics, so behavior-level checks are the trustworthy parity signal.
- [Phase 02]: Quickstart now treats the harness workflow as primary and app launch as a secondary smoke step for this migration slice. — This keeps operational guidance aligned with the harness-first validation posture introduced in the phase.
- [Phase 03]: Orbit camera eye/view math now originates from cglm helpers while legacy vec3_t and mat4_t return paths remain as explicit temporary bridges. — This keeps the visible camera path migrated without widening Phase 3 into a larger caller storage rewrite.
- [Phase 03]: The app assembles view/projection/VP/MVP once through cglm and then copies those matrices into legacy storage for unchanged pick, gizmo, and render consumers. — Explicit bridge copies prevent hidden fallback math3d recomputation while keeping current subsystem boundaries stable.
- [Phase 03]: TransformComp keeps vec3_t and mat4_t cached storage in Phase 3 while local and parented matrix composition moves to cglm raw matrices through memcpy bridge helpers. — This preserves existing render-facing storage and keeps the migration focused on compute hot paths instead of a broader ABI rewrite.
- [Phase 03]: World-point application in ecs_scene.h now flows through one cglm-backed helper instead of leaving mat4_transform_point(t->world_matrix, ...) scattered across creation, update, render, and pick paths. — One shared helper keeps semantics aligned across the dense ECS hot path and gives the harness one real production helper to validate.
- [Phase 03]: The default compare suite now leads with the four Phase 3 hotspot cases so routine harness runs surface the migrated camera and transform parity checks first. — This keeps the current rollout-critical paths visible in strict compare runs and makes parity regressions fail fast.
- [Phase 03]: Quickstart documents Phase 3 as a harness-first macOS workflow with app launch kept explicitly as a manual smoke confirmation step. — Another agent can now rerun the automated gate and the remaining human smoke checklist directly from checked-in docs.
- [Phase 04]: Interaction pick and screen-ray runtime paths now consume one shared helper boundary in src/math/math_interaction.h. — Unifies drag begin/update and pick MVP math behind one cglm-backed implementation surface to reduce drift.
- [Phase 04]: screen-ray helper keeps legacy NDC ray semantics while using cglm matrix inversion/multiplication internals. — Preserved behavior-level parity against frozen pre-migration formulas in strict compare checks.
- [Phase 04]: Axis and plane drag calculations now call one shared helper boundary instead of direct math3d intersection helpers in gizmo runtime paths. — Centralizes gizmo drag math under cglm-backed interaction helpers to reduce runtime drift.
- [Phase 04]: Vertex-mode world/local delta conversion now uses one shared helper with explicit singular-matrix zero fallback. — Ensures deterministic degenerate handling while removing open-coded inverse logic from vertex mode.
- [Phase 04]: Gizmo drag migration is gated by dedicated strict-compare harness cases for axis, plane, and vertex local-delta behavior. — Keeps parity regressions visible and fail-fast in automated harness compare runs.
- [Phase 04]: Quaternion helpers expose project-owned args/struct while delegating operations to cglm glms_quat APIs — Keeps helper surface thin and future-proof without vendor type aliases
- [Phase 04]: Legacy interaction helpers in math3d are now scoped as deprecated in migrated slices — Prevents reintroduction of ray_from_screen/ray_axis_closest_t/ray_plane_intersect into migrated app/pick/gizmo paths
- [Phase 05]: Use persistent Vulkan transfer resources and fence waits for pick readback synchronization in Phase 05 plan 01. — Avoids queue-wide stalls from vkQueueWaitIdle while preserving synchronous hover semantics and existing layout transitions.
- [Phase 05]: Record host-blocked Windows commands as explicit FAIL evidence artifacts rather than omitting required logs/checklists. — Keeps HOT-04 artifact tree complete and auditable even when execution host cannot run Windows toolchains.
- [Phase 05]: Benchmark gate evaluation now runs through `scripts/eval_math_bench.py` with required bench ID enforcement and deterministic `OVERALL` output. — Keeps PERF gate math auditable and reproducible from raw `BENCH ... avg_ns` artifacts.
- [Phase 05]: When Windows MSVC benchmark commands are blocked on this host, capture complete evidence files with provenance notes instead of leaving missing artifacts. — Preserves artifact completeness while explicitly documenting reduced confidence for PERF-03 on non-Windows execution hosts.

### Pending Todos

### Phase 05 backlog handoff

- P1: Rerun HOT-04 strict compare and manual smoke on a Windows MSVC Vulkan host using `.planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/compare.txt` as the current blocked evidence anchor.
- P2: Replace placeholder PERF-03 benchmark provenance with native Windows capture and reevaluate gate output referenced at `.planning/phases/05-windows-vulkan-hardening-and-performance-gates/evidence/windows-vulkan-msvc/candidate/provenance.txt`.
- P2: Scope and schedule TAIL-01 long-tail migration workstream from `.planning/REQUIREMENTS.md` after Windows gate closure work is complete.

### Blockers/Concerns

- Phase 3 execution still needs to keep pick/gizmo bridge inputs stable while camera and transform compute moves onto `cglm`
- Performance claims still need real app-workflow validation on macOS first and Windows Vulkan afterward
- HOT-03 macOS manual interaction smoke is still pending for 04-03 (not executed in headless run).
- HOT-04 runtime acceptance remains blocked on this host; rerun 05-01 command workflow on Windows MSVC Vulkan machine to close gate.
- PERF-03 evidence for 05-02 uses host-blocked placeholder benchmark captures; rerun benchmark capture/eval commands on a Windows MSVC Vulkan host for final sign-off confidence.

## Session Continuity

Last session: 2026-03-25T17:41:56Z
Stopped at: Completed 05-02-PLAN.md
Resume file: .planning/phases/05-windows-vulkan-hardening-and-performance-gates/05-03-PLAN.md
