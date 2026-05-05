# Milestones

## v1.7 Linked Flat JSONL Large-File Refresh Stability (Shipped: 2026-05-05)

**Phases completed:** 2 phases, 5 plans, 10 tasks

**Key accomplishments:**

- Fixed linked flat import startup so observer baselines arm at commit time and initial linked imports settle without self-refresh drift.
- Proved the initial `lamp_11.jsonl` linked import path converges with stable viewport, hierarchy, and slot-buffer state.
- Closed linked refresh collapse/late-churn regressions with exact-footprint slot compaction and cleanup-anomaly fallback.
- Routed linked-root delete through shared cancel-before-delete ordering so refresh teardown and undo restore only committed content.
- Recorded PASS real-file lifecycle evidence for auto-reload, repeated manual refresh, delete after refresh history, and delete while refresh is active.

**Audit:** [v1.7-MILESTONE-AUDIT.md](milestones/v1.7-MILESTONE-AUDIT.md) — tech_debt (all requirements satisfied; `Clear Scene` lifecycle parity and Nyquist validation backfill deferred)

**Archives:**

- [v1.7-ROADMAP.md](milestones/v1.7-ROADMAP.md)
- [v1.7-REQUIREMENTS.md](milestones/v1.7-REQUIREMENTS.md)

---

## v1.6 Observable Flat JSONL Import for Large Geometry Dumps (Shipped: 2026-04-27)

**Phases completed:** 5 phases, 10 plans, 17 tasks

**Key accomplishments:**

- Added a dedicated flat-large JSONL import path with mirrored pre-import options and observer opt-in default OFF.
- Delivered anchor-scoped flat ingest as plain non-sketch entities with deterministic root naming and lazy entry-anchor creation.
- Shipped observer metadata persistence and manual transactional same-root re-import for flat imports.
- Added automatic observer safety behavior (debounce/retry/auto-disable) with bounded single-flight + coalesced rerun semantics.
- Closed repeated-refresh coherence with selection remap, hierarchy guardrails (tree + filter paths), and manual very-large stress evidence.

**Audit:** [v1.6-MILESTONE-AUDIT.md](milestones/v1.6-MILESTONE-AUDIT.md) — tech_debt (no closure blockers; traceability/nyquist metadata cleanup deferred)

**Archives:**

- [v1.6-ROADMAP.md](milestones/v1.6-ROADMAP.md)
- [v1.6-REQUIREMENTS.md](milestones/v1.6-REQUIREMENTS.md)

---

## v1.5 Solver Workflow Robustness + Script Reapply Integrity (Shipped: 2026-04-13)

**Phases completed:** 5 phases, 12 plans, 18 tasks

**Key accomplishments:**

- Delivered script re-apply fidelity preservation for descriptor semantics and color metadata with deterministic repeat-apply behavior.
- Delivered explicit ArcAxisLine and line-end/arc-end tangency coincidence authoring semantics with deterministic lifecycle persistence.
- Closed large-jump PARALLEL/ALONG parity regressions via context-driven authority selection and deterministic bidirectional follow behavior.
- Re-closed deterministic Windows Vulkan milestone gate with canonical baseline+rereun parity evidence and DIAG-03 satisfied verification.
- Shipped JSONL-as-sketch import with persisted observer state, transactional reparse/relink semantics, observer UX relocation, and large-script stability fixes (blank editor/apply crash resolved).

**Audit:** [v1.5-MILESTONE-AUDIT.md](v1.5-MILESTONE-AUDIT.md) — tech_debt (no closure blockers; deferred validation metadata cleanup)

**Archives:**

- [v1.5-ROADMAP.md](milestones/v1.5-ROADMAP.md)
- [v1.5-REQUIREMENTS.md](milestones/v1.5-REQUIREMENTS.md)

---

## v1.4 Solver Robustness + Sketch Gizmo Corrections (Shipped: 2026-04-09)

**Phases completed:** 5 phases, 10 plans, 22 tasks

**Key accomplishments:**

- Delivered pair/group line-line `PARALLEL` and `PERPENDICULAR` constraints with deterministic legality/runtime parity and explicit diagnostics.
- Stabilized line `ALONG X/Y/Z` behavior (including mixed-constraint stacks) through deterministic participant normalization and ordering safeguards.
- Hardened arc-line endpoint tangency drag behavior with transactional rollback semantics, explicit unsatisfied diagnostics, and mirrored interaction parity checks.
- Corrected active-sketch line gizmo behavior to midpoint anchoring with endpoint-authority dragging and grouped undo/redo interaction coherence.
- Published `docs/solver/SOLVER_ARCHITECTURE.md` with practical code anchors, literature references, and a TL;DR debug primer, then cross-linked it from runbooks.
- Closed v1.4 with deterministic Windows Vulkan sign-off using build + canonical seven-test baseline + immediate rerun (`7/7` then `7/7`).

**Audit:** No formal `v1.4-MILESTONE-AUDIT.md` file was present at archival time (user-approved proceed-anyway path).

**Archives:**

- [v1.4-ROADMAP.md](milestones/v1.4-ROADMAP.md)
- [v1.4-REQUIREMENTS.md](milestones/v1.4-REQUIREMENTS.md)

---

## v1.3 Sketch Solver Audit + Constraint Expansion (Shipped: 2026-04-08)

**Phases completed:** 4 phases, 10 plans, 21 tasks

**Key accomplishments:**

- Locked deterministic solver trigger and pass-policy behavior with focused anti-flake reruns on Windows Vulkan.
- Delivered principal-direction ALONG X/Y/Z group constraints across standalone points, line endpoints, and arc landmarks.
- Delivered advanced ARCI constraints (arc-axis, line-end/arc-end tangency, arc endpoint-angle) with deterministic transactional solve semantics.
- Closed ARCI drag-anchor UX so shared-point tangency interactions remain draggable and stable.
- Finalized strict seven-test regression closure gate with mandatory fresh rerun (`7/7` baseline + `7/7` fresh).
- Marked all v1.3 requirements complete (`14/14`), including reliability closure requirement `V13-01`.

**Audit:** No formal `v1.3-MILESTONE-AUDIT.md` file was present at archival time (user-approved proceed-anyway path).

**Archives:**

- [v1.3-ROADMAP.md](milestones/v1.3-ROADMAP.md)
- [v1.3-REQUIREMENTS.md](milestones/v1.3-REQUIREMENTS.md)

---

## v1.2 Sketches, Constraints, Scripting (Shipped: 2026-04-07)

**Phases completed:** 12 phases, 39 plans, 79 tasks

**Key accomplishments:**

- Delivered sketch entity management and GeometryManager bulk workflows with atomic undo semantics.
- Completed full constraint authoring UX closure, including in-context menu legality filtering, glyph interaction, and dimensional edit parity.
- Closed solver control and constrained interaction behavior with deterministic diagnostics and clear-on-success implication lifecycle.
- Shipped standalone Script Editor and Script IO transactional integration with scene API + undo coherence.
- Backfilled script/API verification artifacts and finalized endpoint-driven solve closure evidence (`D-01..D-12`).
- Closed Phase 21 traceability gaps by shipping the SKCH hierarchy refresh fix and promoting SKCH/PH18 requirement alignment.

**Known gaps accepted at archival:**

- Archived `v1.2-MILESTONE-AUDIT.md` status is `gaps_found`; remaining debt is captured in the archived audit artifact and accepted for milestone closure.

**Audit:** [v1.2-MILESTONE-AUDIT.md](milestones/v1.2-MILESTONE-AUDIT.md) — gaps_found (accepted)

**Archives:**

- [v1.2-ROADMAP.md](milestones/v1.2-ROADMAP.md)
- [v1.2-REQUIREMENTS.md](milestones/v1.2-REQUIREMENTS.md)

---

## v1.1 Long-Tail Migration (Shipped: 2026-03-30)

**Phases completed:** 4 phases, 14 plans, 35 tasks

**Key accomplishments:**

- Completed serializer/save-load migration to cglm-backed paths with targeted parity evidence and converter validation.
- Completed JSONL/PLY import migration to cglm-backed paths and stabilized native validation gates.
- Completed undo/editor utility migration and reduced temporary thin-entrypoint glue while preserving behavior parity.
- Closed long-tail validation and boundary finalization with strict compare coverage plus native perf/manual gate evidence.
- Resolved LT-VAL03-02 point-cloud import regression in editable mode and re-validated Windows workflows.
- Finalized Phase 9 verification to `passed (4/4)` and synchronized milestone closure metadata.

**Archives:**

- [v1.1-ROADMAP.md](milestones/v1.1-ROADMAP.md)
- [v1.1-REQUIREMENTS.md](milestones/v1.1-REQUIREMENTS.md)

---

## v1.0 Math Migration (Shipped: 2026-03-26)

**Phases completed:** 5 phases, 15 plans, 30 tasks

**Key accomplishments:**

- Locked and vendored `cglm` `0.9.6` with a project-owned convention/entrypoint contract.
- Added compare/benchmark harness coverage and made it the repeatable migration gate.
- Migrated camera + core transform paths and validated parity on the macOS native flow.
- Migrated interaction math (pick/ray/gizmo) and expanded quaternion/helper surface.
- Closed Windows MSVC Vulkan hardening/performance gates with native evidence and final `Decision: GO`.

**Audit:** [v1.0-MILESTONE-AUDIT.md](milestones/v1.0-MILESTONE-AUDIT.md) — passed

**Archives:**

- [v1.0-ROADMAP.md](milestones/v1.0-ROADMAP.md)
- [v1.0-REQUIREMENTS.md](milestones/v1.0-REQUIREMENTS.md)

---
