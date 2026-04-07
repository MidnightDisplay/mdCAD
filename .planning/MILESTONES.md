# Milestones

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
