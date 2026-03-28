# Phase 9: Long-Tail Validation, Performance Gates, and Boundary Finalization - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Close v1.1 by proving long-tail migration parity/performance confidence and finalizing documentation for the intentionally retained minimal thin-entrypoint boundary. This phase executes validation/performance/manual-gate closure work and boundary finalization only; it does not add new runtime capabilities.

</domain>

<decisions>
## Implementation Decisions

### Validation and gate posture
- **D-01:** Keep the established harness-first validation posture: strict compare + bench + full `math-validation` workflow as the primary automated gate.
- **D-02:** Preserve the phase-level practical gate style from prior phases (targeted evidence artifacts and runbook alignment), while expanding coverage to satisfy `VAL-01`.
- **D-03:** Treat unresolved gate failures in compare/perf/manual smoke as blocking for Phase 9 completion.

### Performance gate policy
- **D-04:** Continue per-case benchmark evaluation (not aggregate-only), consistent with Phase 5 policy.
- **D-05:** Preserve the no-regression threshold posture already used for native gates (`<= 5%` slowdown per benchmark case, rerun policy for marginal noise bands).
- **D-06:** Ensure both native priority targets are explicitly represented in evidence: macOS Metal and Windows MSVC Vulkan.

### Manual smoke closure scope
- **D-07:** Manual smoke closure must cover serializer, importer, undo/redo, and editor interaction workflows as one end-to-end migration confidence pass.
- **D-08:** Manual evidence should use checklist/report artifacts similar to Phases 6-8 for consistency and auditability.

### Thin-entrypoint boundary finalization
- **D-09:** Remaining thin-entrypoint surface must be intentionally minimal, explicitly documented, and mapped to long-term project-owned boundaries (`TRED-02`).
- **D-10:** Boundary docs should clearly distinguish intentionally retained glue vs removable remnants, with rationale and consumer traceability.

### Scope control
- **D-11:** No new feature capabilities, subsystem rewrites, or platform-expansion work (`PLAT-01`, `PLAT-02`) are in scope for Phase 9.

### the agent's Discretion
- Exact coverage additions for compare/harness cases and evidence formatting, as long as `VAL-01/02/03` and `TRED-02` are demonstrably closed.
- Exact sequencing of automation vs manual workflow evidence capture.
- Exact structure/location of final boundary documentation updates, as long as canonical docs remain consistent.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirement contract
- `.planning/ROADMAP.md` — Phase 9 goal, success criteria, and plan slots (`09-01`..`09-03`)
- `.planning/REQUIREMENTS.md` — `TRED-02`, `VAL-01`, `VAL-02`, `VAL-03` requirement intent and traceability
- `.planning/PROJECT.md` — milestone constraints and active migration priorities
- `.planning/STATE.md` — current session routing and milestone status

### Prior decisions that carry forward
- `.planning/phases/05-windows-vulkan-hardening-and-performance-gates/05-CONTEXT.md` — native performance gate policy and evidence expectations
- `.planning/phases/06-serializer-and-save-load-long-tail-migration/06-CONTEXT.md` — light but concrete validation evidence style
- `.planning/phases/07-import-pipeline-long-tail-migration/07-CONTEXT.md` — correctness/evidence discipline for long-tail migration slices
- `.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/08-CONTEXT.md` — glue burn-down outcomes and deferred carry-over to Phase 9

### Validation tooling and runbooks
- `src/math_harness.c` — compare/bench case catalog and strict validation harness entrypoint
- `src/CMakeLists.txt` — native harness target wiring (`mdcad_math_harness`, `math-regression`, `math-bench`, `math-validation`)
- `docs/QUICKSTART.md` — operator workflows for Phase 5-8 validation and the baseline to extend for Phase 9 closure
- `docs/VULKAN_WINDOWS.md` — Windows Vulkan execution and hard-gate operational details

### Risk/quality context
- `.planning/codebase/TESTING.md` — current testing posture and lightweight validation patterns
- `.planning/codebase/CONCERNS.md` — known fragile areas and test-coverage gaps relevant to Phase 9 closure confidence

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/math_harness.c`: established strict compare and benchmark runner with existing long-tail migration coverage and output format already used by prior gate evidence.
- `src/CMakeLists.txt`: already defines reusable native validation targets, enabling phase-level gate runs without new test infrastructure.
- Phase evidence pattern from `.planning/phases/06-*`, `07-*`, `08-*`: targeted checklist + report artifacts that can be extended for cross-surface closure.
- `docs/QUICKSTART.md`: existing operator runbook sections for Phases 5-8 that can absorb a final consolidated Phase 9 workflow.

### Established Patterns
- Harness-first, then manual smoke for user-facing behavior confidence.
- Per-phase evidence artifacts under `.planning/phases/<phase>/evidence/`.
- Scoped, explicit migration closure with requirement mapping and traceability updates.

### Integration Points
- Harness coverage and strict compare/perf outputs integrate through `src/math_harness.c` and existing CMake targets.
- Manual smoke closure connects to runtime workflows through `mdCAD` app execution and phase checklist/report artifacts.
- Final thin-entrypoint boundary documentation integrates with `docs/QUICKSTART.md`, phase evidence docs, and requirement/roadmap traceability surfaces.

</code_context>

<specifics>
## Specific Ideas

- No new specific user requirements were introduced beyond roadmap/requirements-defined Phase 9 closure criteria.
- Open to standard closure approach aligned with prior phase evidence style and gate policy.

</specifics>

<deferred>
## Deferred Ideas

- `PLAT-01` iOS validation and `PLAT-02` web/WASM validation remain deferred to future milestone scope.
- Any broad repo-wide `math3d.h` removal beyond intentional thin-entrypoint finalization remains out of scope.

</deferred>

---

*Phase: 09-long-tail-validation-performance-gates-and-boundary-finalization*
*Context gathered: 2026-03-28*
