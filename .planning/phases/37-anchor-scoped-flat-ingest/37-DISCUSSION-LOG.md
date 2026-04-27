# Phase 37: Anchor-Scoped Flat Ingest - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.  
> Decisions are captured in CONTEXT.md — this log preserves alternatives considered.

**Date:** 2026-04-27
**Phase:** 37-anchor-scoped-flat-ingest
**Areas discussed:** Hierarchy shape, Anchor naming and metadata, Re-import behavior for repeated runs, Post-import selection/focus behavior

---

## Hierarchy shape

| Option | Description | Selected |
|--------|-------------|----------|
| Two-level: root anchor -> per-entry anchors -> geometry entities | Preserves source grouping while keeping entities non-sketch. | ✓ |
| Flat: root anchor -> geometry entities only | Maximally flat structure with no entry grouping. | |
| Hybrid: mostly flat, but create entry anchors only when needed | Conditional grouping model. | |

**User's choice:** Two-level hierarchy.
**Notes:** Additional clarification locked: skip entry anchors for entries with zero importable geometry.

---

## Empty entry handling

| Option | Description | Selected |
|--------|-------------|----------|
| No, skip empty entry anchors | Avoid hierarchy noise from empty entries. | ✓ |
| Yes, always create entry anchors | Mirror source entry list exactly, even if empty. | |

**User's choice:** Skip empty entry anchors.
**Notes:** Keeps the hierarchy lean for large dumps.

---

## Anchor naming and metadata

| Option | Description | Selected |
|--------|-------------|----------|
| Name = filename stem + " (Flat Import)", Description = full source path | Explicit import-type naming. | |
| Name = filename stem only, Description = full source path | Clean naming while retaining source traceability. | ✓ |
| Name = generic "JSONL Flat Import", Description = full source path | Generic naming pattern. | |

**User's choice:** Root name uses filename stem only; description stores full source path.
**Notes:** Entry anchors should keep `Name`/`Description` from source entries.

---

## Entry anchor labels

| Option | Description | Selected |
|--------|-------------|----------|
| Name = entry Name, Description = entry Description | Preserve source-level labels for hierarchy readability. | ✓ |
| Name = entry index, Description = entry Name | Index-focused naming style. | |
| Name = entry Name, Description = blank | Minimal metadata approach. | |

**User's choice:** Name/Description directly from source entry fields.
**Notes:** Matches existing source semantics.

---

## Re-import behavior for repeated runs

| Option | Description | Selected |
|--------|-------------|----------|
| Keep previous anchors and create a new root anchor each run | Non-destructive history across runs. | ✓ |
| Replace the most recent anchor from same source | Single-latest anchor policy. | |
| Prompt user each run (keep/replace) | Interactive policy. | |

**User's choice:** Keep previous anchors and create a new root each run.
**Notes:** Root naming collisions should use numeric suffixes.

---

## Name collision policy

| Option | Description | Selected |
|--------|-------------|----------|
| Append numeric suffix (`name`, `name (2)`, `name (3)`) | Stable, readable deterministic naming. | ✓ |
| Append timestamp | Time-based uniqueness. | |
| Allow duplicate names unchanged | No collision mitigation. | |

**User's choice:** Numeric suffix policy.
**Notes:** Needed because repeated imports preserve older anchors.

---

## Post-import selection/focus behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Select the new root anchor | Auto-focus imported hierarchy. | |
| Keep previous selection | Preserve current editing context. | ✓ |
| Select first created geometry entity | Focus on first imported entity. | |

**User's choice:** Keep previous selection.
**Notes:** No selection side effects should be introduced on success.

---

## the agent's Discretion

- Internal helper structure for collision lookup and empty-entry filtering.
- Exact placement of ingest-structure wiring between import job and UI boundary.

## Deferred Ideas

None.
