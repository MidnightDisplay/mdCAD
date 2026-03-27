# Phase 7: Import Pipeline Long-Tail Migration - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-27
**Phase:** 07-import-pipeline-long-tail-migration
**Areas discussed:** Import transform math policy, Importer boundary and helper placement, Parity validation depth and evidence shape, Risk handling for large-file imports

---

## Import transform math policy

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve existing transform order/behavior | Keep CoM shift -> rotate -> scale semantics unchanged | |
| Adjust transform semantics for correctness | Allow behavior corrections when current math is weak/incorrect | ✓ |
| Agent discretion | Let implementation decide under parity/correctness constraints | ✓ (follow-up) |

**User's choice:** Correctness-driven adjustment; detailed semantic selection delegated to implementation based on code reality.  
**Notes:** User explicitly set correctness-first posture with evidence notes for any visible behavior changes.

## Importer boundary and helper placement

| Option | Description | Selected |
|--------|-------------|----------|
| Shared `src/math/` helpers where practical | Importer-local only for importer-specific behavior | ✓ |
| Keep migration importer-local only | No shared helper migration in this phase | |
| Agent discretion | Let implementation decide entirely | |

**User's choice:** Shared helpers where practical, importer-local only when clearly specific.  
**Notes:** User also selected migration of both parser/loader and import-job math surfaces in Phase 7.

## Parity validation depth and evidence shape

| Option | Description | Selected |
|--------|-------------|----------|
| Light gate | Compile + targeted checks | ✓ |
| Expanded gate | Broader new harness automation in this phase | |
| Agent discretion | Decide evidence depth during implementation | |

**User's choice:** Light gate with targeted checks.  
**Notes:** Required recorded outcomes locked to placement, orientation, scale, entity/triangle counts, and parenting structure for representative + variant samples.

## Risk handling for large-file imports

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve chunked behavior and treat regressions as blockers | Keep progress/throughput semantics stable | ✓ |
| Focus on correctness only; defer perf/behavior risks | Allow migration-induced large-file drift | |
| Agent discretion | Decide risk posture during implementation | |

**User's choice:** Preserve chunked behavior; regressions are blockers for phase completion.  
**Notes:** Scope guard confirmed: new importer capabilities are deferred.

## the agent's Discretion

- Exact correctness fixes chosen per importer path after code-level analysis.
- Exact helper API shape and internal split between shared and importer-local logic.
- Exact targeted sample fixtures and evidence formatting.

## Deferred Ideas

- New importer capabilities/formats outside current JSONL/PLY math migration boundary.

