# Milestones

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
