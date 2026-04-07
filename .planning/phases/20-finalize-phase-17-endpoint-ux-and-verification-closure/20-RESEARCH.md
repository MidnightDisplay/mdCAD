# Phase 20: finalize-phase-17-endpoint-ux-and-verification-closure - Research

**Researched:** 2026-04-07  
**Domain:** Phase 17 verification closure (endpoint UX + requirement evidence reconciliation)  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Evidence strategy
- **D-01:** Default policy is to primarily reuse existing approved Phase 17 evidence (`17-UAT.md`, prior command evidence, prior summaries), with only minimal reruns where needed.
- **D-02:** Every reused requirement row must include explicit source citations (artifact path + specific section/row reference).
- **D-03:** If any row remains ambiguous after reuse, escalate that row to fresh targeted rerun and/or fresh manual recheck before marking `passed`.

### Verification artifact structure
- **D-04:** `17-VERIFICATION.md` is the authoritative closure artifact for Phase 17.
- **D-05:** `17-VALIDATION.md` remains a supporting strategy/evidence log and must be aligned with final closure state.
- **D-06:** `17-UAT.md` remains the executed manual proof artifact and should be preserved as canonical manual evidence.
- **D-07:** `17-VERIFICATION.md` must explicitly represent all `D-01..D-12` as row-level entries with evidence links.

### Scope and execution guardrails
- **D-08:** Use docs-first closure. Apply code changes only if a required closure check fails and the failure is reproducible.
- **D-09:** Any such code fix must be minimal and directly tied to unblocking a required closure row.

### Manual sign-off policy
- **D-10:** Reuse `17-UAT.md` tests 1-7 as the mandatory manual sign-off set.
- **D-11:** Run fresh manual/rerun evidence only for rows that become ambiguous or fail under current validation.

### the agent's Discretion
- Define the minimal rerun command set used to resolve ambiguous rows, as long as it remains auditable and requirement-linked.
- Choose exact verification table formatting and citation layout, while preserving per-row traceability.
- Decide whether a failing row needs command rerun, manual rerun, or both, based on reproducibility and evidence sufficiency.

### Deferred Ideas (OUT OF SCOPE)
- Opportunistic endpoint UX polish beyond failing closure checks (future scope).
- Broader residual traceability cleanup outside `D-01..D-12` (handled by Phase 21).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| D-01 | Recalculate validates full active sketch constraints before committing geometry updates | Reuse `17-VALIDATION.md` Decision-to-Evidence row + `scene_solver_contract` evidence; require explicit row citation in new `17-VERIFICATION.md`. |
| D-02 | Successful solve attempts apply resulting geometry updates atomically and immediately | Reuse `scene_solver_contract` commit-on-success evidence from existing validation artifacts; verify citation quality and rerun only if ambiguous. |
| D-03 | Drag interactions use live constrained solve projection when satisfiable | Reuse `scene_solver_drag` feasible projection evidence; map row to test and command transcript in `17-VERIFICATION.md`. |
| D-04 | Unsatisfiable drag keeps last valid solved state and surfaces immediate feedback | Reuse `scene_solver_drag` + manual viewport checks from `17-UAT.md`; escalate to manual rerun if row is ambiguous. |
| D-05 | Contradictory solves fail deterministically with no scene mutation and stable implication payload | Reuse `scene_solver_contract`/`scene_solver_diagnostics` evidence and implication-order references; rerun targeted tests for any unclear claim. |
| D-06 | Solver diagnostics dedupe identical consecutive entries while preserving append order | Reuse `scene_solver_diagnostics` evidence and supporting UI behavior notes in `17-VALIDATION.md`. |
| D-07 | Live solve path uses bounded per-frame budget with graceful degradation contract | Reuse bounded projection assertions from `scene_solver_drag`; ensure row-level reference to budget/degrade contract. |
| D-08 | Deterministic solver regression fixtures are wired as automated acceptance gates | Reuse aggregate gate command pattern (`scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics`) with explicit pass evidence references. |
| D-09 | Constraint participant representation supports endpoint/sub-entity metadata | Reuse endpoint-native entity model evidence from `17-04/17-05` summaries and `endpoint_pick_test.c` coverage. |
| D-10 | Endpoint controls are first-class selectable/pickable participants for Coincident authoring | Reuse `17-UAT` Test 1/2/3 outcomes + endpoint_pick automated coverage (`point_context_*`, pick precedence tests). |
| D-11 | Coincident authoring supports endpoint-to-endpoint participant semantics through direct selection model | Reuse `17-UAT` chain/loop evidence + endpoint context legality tests and plan 17-05 remediation artifacts. |
| D-12 | Pick/render layering maintains endpoint selection priority over continuous primitives | Reuse overlay precedence and collision determinism tests in `endpoint_pick_test.c` plus `17-UAT` endpoint selection evidence. |
</phase_requirements>

## Summary

Phase 20 is an **audit-grade closure phase** for Phase 17, not a feature-building phase. The core deliverable is a new authoritative `.planning/phases/17-constraint-driven-geometry-solving/17-VERIFICATION.md` that covers every requirement `D-01..D-12` with row-level citations to existing approved evidence, plus targeted fresh reruns/manual checks only where evidence is ambiguous.

The existing repository already contains strong supporting evidence: `17-VALIDATION.md` has mapped test coverage and command history, `17-UAT.md` reports 7/7 pass for endpoint UX checks, and `17-05-SUMMARY.md` captures endpoint remediation and follow-up stability work. The milestone audit still flags Phase 17 as orphaned because the authoritative verification artifact is missing—not because the behavior necessarily lacks implementation/testing.

Plan Phase 20 as a **documentation-first reconciliation workflow**: build a strict requirement matrix in `17-VERIFICATION.md`, align `17-VALIDATION.md` closure language with final dispositions, and only apply minimal code changes if a reproducible closure check now fails.

**Primary recommendation:** Create `17-VERIFICATION.md` with explicit `D-01..D-12` rows and source-linked evidence first; run only targeted reruns/manual rechecks for rows that cannot be unambiguously closed from existing artifacts.

## Project Constraints (from copilot-instructions.md)

No `./copilot-instructions.md` file exists in repository root at research time, so no additional directives were extracted from that source.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | 4.3.0 (local) | Build selected test targets when reruns are required | Canonical project build system and existing validation command baseline |
| CTest | 4.3.0 (local) | Targeted requirement verification reruns | Existing Nyquist validation workflow and prior Phase 17 evidence already use CTest |
| Phase artifacts (`17-VALIDATION.md`, `17-UAT.md`, `17-05-SUMMARY.md`) | In-repo | Canonical evidence sources for reuse-and-cite strategy | Locked decisions explicitly require reuse with citations before reruns |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `endpoint_pick` CTest target | In-repo | Endpoint pick/context/layering regression evidence for D-09..D-12 | Use for ambiguous/failing endpoint closure rows |
| `scene_solver_contract` / `scene_solver_drag` / `scene_solver_diagnostics` | In-repo | Deterministic solver behavior evidence for D-01..D-08 | Use for ambiguous core-solver rows or to refresh stale evidence |
| Windows MSVC + Vulkan build tree (`build-vulkan`) | Existing local tree | Gate-platform continuity with prior phase evidence | Use when fresh command evidence is required |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Reuse-first with targeted reruns | Full-suite reruns for all rows | More expensive and conflicts with locked docs-first/minimal-rerun posture |
| Requirement-row citations | Aggregate references only | Fails explicit D-02 per-row traceability requirement |

**Installation:**
```bash
# No new dependencies required for Phase 20.
# Uses existing CMake/CTest toolchain and current repo artifacts.
```

## Architecture Patterns

### Recommended Project Structure
```text
.planning/phases/17-constraint-driven-geometry-solving/
├── 17-VERIFICATION.md   # NEW authoritative D-01..D-12 closure matrix
├── 17-VALIDATION.md     # supporting strategy/evidence log aligned to final closure
├── 17-UAT.md            # canonical manual test artifact (preserved, cited)
└── 17-05-SUMMARY.md     # remediation narrative and targeted command evidence
```

### Pattern 1: Authoritative requirement-first verification matrix
**What:** One row per `D-01..D-12` with status, implementation anchor, automated evidence, manual evidence link, and closure rationale.  
**When to use:** Always for Phase 20 closure output (`17-VERIFICATION.md`).  
**Example:**
```markdown
| Requirement | Status | Automated Evidence | Manual Evidence | Citation |
|-------------|--------|--------------------|-----------------|----------|
| D-10 | passed | `ctest -R endpoint_pick ...` (PASS) | `17-UAT.md` Test 1-3 pass | `17-UAT.md` lines 23-34 |
```

### Pattern 2: Ambiguity escalation protocol
**What:** If reused evidence is unclear, rerun only the smallest command/manual set required to close that row.  
**When to use:** Any row with uncertain citation quality or conflicting artifacts.  
**Example:**
```text
D-12 ambiguous -> rerun:
1) ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure
2) Manual recheck of endpoint layering from 17-UAT test steps
```

### Pattern 3: Supporting artifact alignment
**What:** Keep `17-VALIDATION.md` consistent with final verification disposition without changing its role as strategy/evidence support.  
**When to use:** After `17-VERIFICATION.md` statuses are finalized.  
**Example:**
```markdown
- Update open/conditional notes in `17-VALIDATION.md` to reflect final approved closure state.
- Preserve checkpoint history sections as historical trail (do not rewrite history).
```

### Anti-Patterns to Avoid
- **Scope creep closure:** Adding unrelated UX polish while closing evidence debt.
- **Uncited reuse claims:** Marking `passed` without row-level source links.
- **Over-rerun behavior:** Running full suites by default when row-level ambiguity does not require it.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Verification schema | Ad-hoc prose-only closure notes | Requirement matrix in `17-VERIFICATION.md` | Auditors need row-level traceability and deterministic status review |
| Endpoint proof set | New custom endpoint test harness | Existing `endpoint_pick` target + `17-UAT.md` | Coverage already exists for pick legality/layering and UX checkpoints |
| Solver closure gate | New synthetic acceptance script | Existing CTest target set from `17-VALIDATION.md` | Already mapped to D-01..D-08 and consistent with Nyquist flow |

**Key insight:** Phase 20 should synthesize and reconcile existing evidence into an authoritative artifact; it should not introduce new verification infrastructure unless a real evidence gap is discovered.

## Common Pitfalls

### Pitfall 1: “Missing artifact” misread as “missing implementation”
**What goes wrong:** Team treats the audit gap as a feature gap and starts coding.  
**Why it happens:** v1.2 audit reports requirements as orphaned when `17-VERIFICATION.md` is absent.  
**How to avoid:** Close artifact traceability first; code changes only if reproducible checks fail.  
**Warning signs:** New runtime commits appear before `17-VERIFICATION.md` matrix draft exists.

### Pitfall 2: Incomplete citations in reused rows
**What goes wrong:** Reused evidence is referenced generally, not precisely.  
**Why it happens:** Fast closure effort skips section/row-level anchors.  
**How to avoid:** Every row includes artifact path + exact section/row/test reference.  
**Warning signs:** Reviewer must search manually to find the claimed evidence.

### Pitfall 3: Conditional text left unresolved in validation docs
**What goes wrong:** `17-VALIDATION.md` still reads as pending even after closure work.  
**Why it happens:** Final disposition is recorded only in one artifact.  
**How to avoid:** Synchronize closure language across `17-VERIFICATION.md` and `17-VALIDATION.md`.  
**Warning signs:** One doc says passed while another says pending/conditional.

## Code Examples

Verified patterns from in-repo sources:

### Constraint legality filtering at menu generation
```c
// Source: src/app.c
for (int t = 0; t < CONSTRAINT_TYPE_COUNT; t++) {
    constraint_type_t type = (constraint_type_t)t;
    if (!constraint_type_is_selection_legal(&state.constraint_menu_signature, type)) continue;
    ...
}
```

### Endpoint point context legality tests
```c
// Source: src/tests/endpoint_pick_test.c
test_endpoint_point_context_filters_line_only_constraints
test_endpoint_point_context_keeps_coincident_for_endpoint_pairs
```

### Endpoint layering precedence tests
```c
// Source: src/tests/endpoint_pick_test.c
test_endpoint_pick_overlay_precedence_contract
test_endpoint_pick_collision_prefers_overlay
test_endpoint_pick_collision_deterministic_repeated_sampling
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Phase summary/validation without authoritative phase verification artifact | Mandatory requirement-level `*-VERIFICATION.md` for audit closure | Enforced by v1.2 milestone audit (2026-04-05) | Prevents orphaned requirement status in milestone traceability |
| Endpoint UX status tracked as conditional in validation history | Explicit closure via final verification matrix with linked evidence | Phase 20 scope definition (2026-04-07) | Clear pass/fail ownership per D-01..D-12 |

**Deprecated/outdated:**
- “Phase 17 is closeable without `17-VERIFICATION.md`” — outdated under current audit gate.

## Open Questions

1. **Do any D-rows remain ambiguous after strict citation pass?**
   - What we know: `17-UAT.md` is 7/7 pass and validation maps all D-rows.
   - What's unclear: Whether each row can be cited at required granularity without fresh rerun.
   - Recommendation: Perform citation-first matrix draft, then rerun only flagged rows.

2. **Should one fresh aggregate solver gate rerun be included even if no rows are ambiguous?**
   - What we know: Locked decisions prefer minimal reruns.
   - What's unclear: Whether reviewer expects freshness across all D-rows for closure date.
   - Recommendation: Keep optional; run only if reviewer policy or evidence ambiguity requires it.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Build targeted test executables when reruns needed | ✓ | 4.3.0 | — |
| ctest | Targeted closure reruns (`endpoint_pick`, solver tests) | ✓ | 4.3.0 | — |
| git | Evidence provenance and artifact diff review | ✓ | 2.51.1.windows.1 | — |
| node | GSD utility scripts | ✓ | v25.8.1 | — |
| `build-vulkan` tree | Existing command baseline in phase artifacts | ✓ | existing configured dir | Reconfigure if cache is stale |

**Missing dependencies with no fallback:**
- None identified.

**Missing dependencies with fallback:**
- None identified.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C test executables |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` |
| Full suite command | `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| D-01 | Full-constraint validation before commit | contract | `ctest -R scene_solver_contract --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-02 | Atomic immediate apply on solve success | contract | `ctest -R scene_solver_contract --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-03 | Live satisfiable constrained drag projection | integration | `ctest -R scene_solver_drag --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-04 | Unsat drag preserves last valid solved state with feedback | integration + manual | `ctest -R scene_solver_drag --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-05 | Deterministic contradictory failure/no mutation | contract/integration | `ctest -R "scene_solver_contract|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-06 | Diagnostics consecutive dedupe + append order | unit/integration | `ctest -R scene_solver_diagnostics --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-07 | Bounded per-frame solve budget/degrade path | integration | `ctest -R scene_solver_drag --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-08 | Deterministic regression fixtures as acceptance gates | suite | `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-09 | Endpoint/sub-entity metadata participant model | integration + manual | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-10 | Endpoint points first-class selectable/pickable | integration + manual | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-11 | Endpoint-to-endpoint Coincident direct authoring | integration + manual | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| D-12 | Endpoint pick/render priority over primitives | integration + manual | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ |

### Sampling Rate
- **Per task commit:** `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` (or requirement-specific subset)
- **Per wave merge:** `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure`
- **Phase gate:** All closure rows in `17-VERIFICATION.md` are `passed` with citations; required targeted reruns/manual rechecks complete

### Wave 0 Gaps
None — test infrastructure and target registration already exist (`scene_solver_contract`, `scene_solver_drag`, `endpoint_pick`, `scene_solver_diagnostics` in `src/CMakeLists.txt`).

## Sources

### Primary (HIGH confidence)
- `.planning/phases/20-finalize-phase-17-endpoint-ux-and-verification-closure/20-CONTEXT.md` — locked decisions and closure scope
- `.planning/REQUIREMENTS.md` — D-01..D-12 definitions and traceability ownership
- `.planning/ROADMAP.md` — Phase 20 goal/dependency/gap closure contract
- `.planning/v1.2-MILESTONE-AUDIT.md` — authoritative blocker source (missing Phase 17 verification artifact)
- `.planning/phases/17-constraint-driven-geometry-solving/17-VALIDATION.md` — existing D-row test mapping, command history, and open/conditional notes
- `.planning/phases/17-constraint-driven-geometry-solving/17-UAT.md` — manual baseline tests 1-7 with pass results
- `.planning/phases/17-constraint-driven-geometry-solving/17-05-SUMMARY.md` — endpoint remediation and targeted regression evidence
- `src/CMakeLists.txt` — CTest target registration for solver and endpoint suites
- `src/tests/endpoint_pick_test.c` — endpoint context/layering/sync regression anchors
- `src/constraints/constraint_types.h` — legality filtering contract
- `src/app.c` — constraint context assembly/menu legality application flow

### Secondary (MEDIUM confidence)
- `.planning/STATE.md` — continuity and phase ordering context

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — directly validated against local tool availability and in-repo build/test configuration.
- Architecture: **HIGH** — constrained by explicit locked decisions in `20-CONTEXT.md` and prior phase closure patterns.
- Pitfalls: **HIGH** — grounded in audit findings and known prior Phase 17 checkpoint history.

**Research date:** 2026-04-07  
**Valid until:** 2026-05-07
