# Phase 19: Backfill Script/API Verification Artifacts - Context

**Gathered:** 2026-04-05
**Status:** Ready for planning

<domain>
## Phase Boundary

Close missing verification coverage for Phase 13 and Phase 14 script/API requirements by creating auditable verification artifacts and evidence parity for:

- `SCRP-01`, `SCRP-02`, `SCRP-03`, `SCRP-04`, `SCRP-05`, `SCRP-06`
- `API-01`, `API-02`

This phase is verification-closure work, not feature expansion.

</domain>

<decisions>
## Implementation Decisions

### Evidence freshness level
- **D-01:** Use **fresh targeted reruns** plus new verification docs as the default closure strategy.
- **D-02:** Prefer targeted script/API verification commands over mandatory full-suite rebuilds unless a specific closure row requires broader coverage.

### Manual evidence policy
- **D-03:** Reuse previously approved manual/UAT outcomes where acceptance already happened, but require explicit cross-links to the originating evidence artifacts.
- **D-04:** Do not invent manual pass claims; every reused manual claim must cite concrete prior checkpoint/UAT records.

### Verification status normalization
- **D-05:** Requirements previously blocked by `human_needed` may be upgraded to `passed` when implementation and acceptance are already evidenced, with explicit traceability links.
- **D-06:** Status upgrades must include rationale in verification docs so audit readers can reconstruct why closure is valid without rerunning the full history.

### Scope boundary enforcement
- **D-07:** Keep Phase 19 strictly scoped to Phase 13/14 script+API verification closure (`SCRP-01..06`, `API-01..02`).
- **D-08:** No new runtime behavior/features are introduced in this phase.

### Prep notes for downstream closure
- **D-09:** Include concise prep notes for Phase 20/21 only as handoff context (not as implemented scope in Phase 19).

### the agent's Discretion
- Exact command mix for targeted reruns (`script_roundtrip_tests`-focused smoke/gate variants) as long as evidence remains auditable.
- Exact structure of the new verification documents (tables/sections/frontmatter), provided requirement traceability is explicit and complete.
- Exact wording of cross-artifact status-upgrade rationale, provided it is objective and source-linked.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone contract and gap source
- `.planning/ROADMAP.md` — Phase 19 goal/scope/dependencies and required requirement IDs.
- `.planning/REQUIREMENTS.md` — requirement definitions and current traceability statuses for `SCRP-*` and `API-*`.
- `.planning/v1.2-MILESTONE-AUDIT.md` — authoritative gap list and evidence expectations driving this closure phase.
- `.planning/PROJECT.md` — v1.2 scope and non-negotiables (no feature creep during closure phases).
- `.planning/STATE.md` — continuity baseline and prior phase decisions affecting validation posture.

### Upstream script/API phase decisions and evidence
- `.planning/phases/13-script-round-trip-baseline/13-CONTEXT.md` — locked Phase 13 scripting decisions.
- `.planning/phases/13-script-round-trip-baseline/13-VALIDATION.md` — prior Phase 13 validation contract and command expectations.
- `.planning/phases/13-script-round-trip-baseline/13-03-SUMMARY.md` — accepted Script Editor closure evidence and manual checkpoint outcomes.
- `.planning/phases/14-script-io-api-undo-integration/14-CONTEXT.md` — locked Phase 14 script IO/API/undo decisions.
- `.planning/phases/14-script-io-api-undo-integration/14-VALIDATION.md` — prior Phase 14 validation contract and manual verification rows.
- `.planning/phases/14-script-io-api-undo-integration/14-03-SUMMARY.md` — accepted Script IO/API transactional closure and UAT stabilization outcomes.

### Product intent and behavior contract
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — canonical scripting/API behavior intent for v1.2.
- `CHECKPOINT.md` — ongoing continuity artifact and acceptance narrative anchor.

### Code and test anchors for targeted reruns
- `src/CMakeLists.txt` — CTest target registration (`script_roundtrip_tests`) and test-gate wiring.
- `src/tests/script_roundtrip_tests.c` — script parser/apply/IO/undo regression coverage used for targeted evidence refresh.
- `src/ecs/ecs_scene.h` — script façade APIs (`scene_script_preview_parse`, `scene_script_apply_commit`, `scene_script_io_*`, `scene_script_reemit_for_sketch`).
- `src/undo_redo.h` — undo command contracts including script transaction command type.
- `src/undo_redo_exec.h` — script transaction record/replay logic (`CMD_SCRIPT_APPLY_TRANSACTION`) and side-effect boundaries.
- `src/app.c` — Script Editor/Script IO window flows and manual behavior surfaces referenced by reused manual evidence.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `script_roundtrip_tests` already exercises most script/API closure surfaces (apply atomicity, preview safety, IO edits, undo transaction semantics).
- Scene scripting façades in `src/ecs/ecs_scene.h` provide stable integration points for verification claims without adding new behavior.
- Existing Phase 13/14 summaries include accepted manual verification outcomes that can be reused with explicit citations.

### Established Patterns
- Verification phases use auditable command evidence plus human-check references; unsupported pass assertions are rejected.
- Script mutations are treated transactionally (`scene_script_apply_commit` + `CMD_SCRIPT_APPLY_TRANSACTION`) and should be validated as such.
- Windows MSVC + Vulkan is the active gate for closure evidence.

### Integration Points
- Produce missing verification artifacts for phases 13 and 14 with requirement-by-requirement status/evidence mapping.
- Run targeted reruns (script/API-focused) and capture fresh output for closure rows.
- Cross-link reused manual evidence from prior accepted summaries/checkpoints.
- Add short handoff notes for Phase 20/21 where remaining audit work intersects later closure phases.

</code_context>

<specifics>
## Specific Ideas

- Favor "fresh targeted rerun + explicit evidence linking" over purely archival backfill.
- Keep this phase crisp: close audit gaps for script/API verification artifacts, then hand off endpoint/traceability debt to Phases 20/21.
- Use explicit "status-upgrade rationale" text where prior `human_needed` rows become `passed` through linked evidence.

</specifics>

<deferred>
## Deferred Ideas

- Any new scripting/API features beyond closure evidence belong in future feature phases, not Phase 19.
- Broader milestone cleanup outside `SCRP-*` and `API-*` closure belongs to Phases 20 and 21.

</deferred>

---

*Phase: 19-backfill-script-api-verification-artifacts*
*Context gathered: 2026-04-05*
