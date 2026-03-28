# VAL-01 coverage map

## Scope and gate policy

- Requirement: `VAL-01` — compare harness coverage includes long-tail migration touchpoints and passes strict checks on required parity cases.
- Gate posture: Harness-first closure (D-01) with practical, auditable mapping artifacts (D-02).
- Blocking rule: Any strict compare failure in required rows is a phase blocker (D-03).
- Scope guard: No platform-expansion coverage rows (`PLAT-01`, `PLAT-02`) in this plan per D-11.

## Touchpoint → compare/manual traceability

| Touchpoint | Requirement slice | Compare case ID(s) | Manual fallback needed? | Rationale |
|---|---|---|---|---|
| Serializer transform encode/decode parity | serializer | `serializer-transform-parity`, `transform-compose` | No | Serializer paths rely on transform composition correctness; explicit serializer parity case validates matrix + sample point transforms. |
| Import placement/orientation parity | import | `import-hierarchy-parity`, `hierarchy-world-transform`, `view-projection-roundtrip` | No | Import output depends on hierarchy/world transforms and projection-visible placement; dedicated import hierarchy case closes long-tail mapping. |
| Undo/redo axis drag parity | undo | `undo-axis-drag-parity`, `gizmo-axis-drag` | No | Undo replay correctness requires stable axis drag deltas; dedicated undo axis case checks start/update/total parity. |
| Editor plane drag parity | editor | `editor-plane-drag-parity`, `gizmo-plane-drag`, `gizmo-vertex-local-delta` | No | Editor interactions require plane hit behavior and local delta conversion parity; dedicated editor plane case plus existing drag/local-delta checks provide full coverage. |
| Shared interaction ray construction used by editor/import tooling | editor, import | `screen-ray-unproject`, `pick-mvp-window` | No | Screen-ray and pick MVP parity are foundational for viewport interaction semantics across long-tail workflows. |
| Quaternion-driven orientation parity used by imported/editable transforms | import, editor | `quat-rotate-vector`, `quat-compose-order` | No | Quaternion parity preserves orientation behavior for migration-sensitive transform consumers. |

## Evidence links

- Strict compare execution proof: `evidence/coverage/compare-strict.txt`
- Harness case catalog source: `src/math_harness.c` (`mdcad_compare_cases`)

## Closure statement

`VAL-01` coverage is explicit for serializer/import/undo/editor touchpoints with no unmapped required surfaces in this plan. Strict compare evidence is current and green; unresolved failures would block closure by policy (D-03).
